#include <gtest/gtest.h>
#include <vector>
#include <cstring>
#include <cerrno>
extern "C" {
#include "dns_server.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
}
namespace {
TaskFunction_t worker;
EventBits_t bits;
bool fail_group, fail_socket, fail_task, fail_bind, fail_close, fail_recv, fail_send, short_send;
bool run_early, nested_start, defer_wait, run_at_wait;
int options_fail_at, options_calls, socket_calls, close_calls, task_calls, bind_calls, send_calls;
int delete_calls, transient_recv, transient_send;
std::vector<std::vector<uint8_t>> packets;
std::vector<uint8_t> sent;
std::vector<uint8_t> valid_query() {
    return {0,1,1,0,0,1,0,0,0,0,0,0,1,'x',0,0,1,0,1};
}
void run_worker() { ASSERT_NE(worker,nullptr); auto fn=worker; worker=nullptr; fn(nullptr); }
class ReadPicoDnsServer : public testing::Test {
    void SetUp() override {
        worker=nullptr; bits=0; fail_group=fail_socket=fail_task=fail_bind=fail_close=fail_recv=fail_send=short_send=false;
        run_early=nested_start=defer_wait=run_at_wait=false;
        options_fail_at=options_calls=socket_calls=close_calls=task_calls=bind_calls=send_calls=delete_calls=0;
        transient_recv=transient_send=0; packets.clear(); sent.clear();
    }
};
}
extern "C" {
const char *esp_err_to_name(esp_err_t) { return "synthetic"; }
EventGroupHandle_t xEventGroupCreate(void) { return fail_group?nullptr:&bits; }
EventBits_t xEventGroupSetBits(EventGroupHandle_t,EventBits_t mask) { return bits|=mask; }
EventBits_t xEventGroupClearBits(EventGroupHandle_t,EventBits_t mask) { auto old=bits; bits&=~mask; return old; }
EventBits_t xEventGroupWaitBits(EventGroupHandle_t,EventBits_t mask,BaseType_t,BaseType_t,TickType_t timeout) {
    EXPECT_EQ(timeout,2000);
    if (!defer_wait && run_at_wait && worker) run_worker();
    return bits&mask;
}
BaseType_t xTaskCreate(TaskFunction_t fn,const char *,uint32_t stack,void *,unsigned,TaskHandle_t *out) {
    ++task_calls; EXPECT_EQ(stack,4096u); EXPECT_EQ(out,nullptr);
    if (nested_start) EXPECT_EQ(dns_server_start(),ESP_ERR_INVALID_STATE);
    if (fail_task) return pdFALSE;
    worker=fn;
    if (run_early) run_worker();
    return pdPASS;
}
void vTaskDelay(TickType_t) {}
void vTaskDelete(TaskHandle_t h) { EXPECT_EQ(h,nullptr); ++delete_calls; }
int dns_test_socket(int family,int type,int protocol) {
    ++socket_calls; EXPECT_EQ(family,AF_INET); EXPECT_EQ(type,SOCK_DGRAM); EXPECT_EQ(protocol,IPPROTO_UDP);
    return fail_socket?-1:42;
}
int dns_test_bind(int fd,const struct sockaddr *addr,socklen_t size) {
    ++bind_calls; EXPECT_EQ(fd,42); EXPECT_EQ(size,sizeof(sockaddr_in));
    auto p=reinterpret_cast<const sockaddr_in *>(addr);
    EXPECT_EQ(ntohs(p->sin_port),53); EXPECT_EQ(ntohl(p->sin_addr.s_addr),0xc0a80401u);
    return fail_bind?-1:0;
}
int dns_test_setsockopt(int fd,int level,int option,const void *value,socklen_t size) {
    ++options_calls; EXPECT_EQ(fd,42); EXPECT_EQ(level,SOL_SOCKET);
    EXPECT_TRUE(option==SO_RCVTIMEO || option==SO_SNDTIMEO); EXPECT_EQ(size,sizeof(timeval));
    auto timeout=reinterpret_cast<const timeval *>(value); EXPECT_EQ(timeout->tv_usec,250000);
    return options_fail_at==options_calls?-1:0;
}
ssize_t dns_test_recvfrom(int fd,void *buffer,size_t size,int,struct sockaddr *client,socklen_t *length) {
    EXPECT_EQ(fd,42); EXPECT_EQ(size,513u); EXPECT_EQ(*length,sizeof(sockaddr_in));
    auto p=reinterpret_cast<sockaddr_in *>(client); p->sin_family=AF_INET;
    if (transient_recv>0) { --transient_recv; errno=EINTR; return -1; }
    if (fail_recv || packets.empty()) { errno=EIO; return -1; }
    auto packet=packets.front(); packets.erase(packets.begin());
    size_t n=std::min(size,packet.size()); memcpy(buffer,packet.data(),n); return n;
}
ssize_t dns_test_sendto(int fd,const void *data,size_t size,int,const struct sockaddr *,socklen_t) {
    ++send_calls; EXPECT_EQ(fd,42); EXPECT_LE(size,512u);
    if (transient_send>0) { --transient_send; errno=EAGAIN; return -1; }
    if (fail_send) { errno=EIO; return -1; }
    if (short_send) return size-1;
    sent.assign((const uint8_t *)data,(const uint8_t *)data+size); return size;
}
int dns_test_close(int fd) { EXPECT_EQ(fd,42); ++close_calls; return fail_close?-1:0; }
}

TEST_F(ReadPicoDnsServer, StartOnlySucceedsAfterSocketOptionsBindAndTaskCreation) {
    EXPECT_EQ(dns_server_start(),ESP_OK); EXPECT_EQ(options_calls,2); EXPECT_EQ(bind_calls,1); EXPECT_EQ(task_calls,1);
    EXPECT_EQ(close_calls,0); EXPECT_EQ(dns_server_start(),ESP_OK); EXPECT_EQ(socket_calls,1);
    run_at_wait=true; EXPECT_EQ(dns_server_stop_checked(),ESP_OK); EXPECT_EQ(close_calls,1); EXPECT_EQ(delete_calls,1);
    EXPECT_EQ(dns_server_stop_checked(),ESP_OK); EXPECT_EQ(close_calls,1);
    EXPECT_EQ(dns_server_start(),ESP_OK); EXPECT_EQ(socket_calls,2);
}
TEST_F(ReadPicoDnsServer, GroupAllocationFailureNeverOpensSocketAndExplicitRetryWorks) {
    fail_group=true; EXPECT_EQ(dns_server_start(),ESP_ERR_NO_MEM); EXPECT_EQ(socket_calls,0);
    fail_group=false; EXPECT_EQ(dns_server_start(),ESP_OK);
}
TEST_F(ReadPicoDnsServer, SocketFailureNeverCreatesTaskOrClosesUnownedDescriptor) {
    fail_socket=true; EXPECT_EQ(dns_server_start(),ESP_FAIL); EXPECT_EQ(close_calls,0); EXPECT_EQ(task_calls,0);
    fail_socket=false; EXPECT_EQ(dns_server_start(),ESP_OK);
}
TEST_F(ReadPicoDnsServer, TimeoutOptionFailureClosesExactlyOnceBeforeExplicitRetry) {
    options_fail_at=1; EXPECT_EQ(dns_server_start(),ESP_FAIL); EXPECT_EQ(close_calls,1); EXPECT_EQ(bind_calls,0);
    options_fail_at=0; EXPECT_EQ(dns_server_start(),ESP_OK); EXPECT_EQ(task_calls,1);
}
TEST_F(ReadPicoDnsServer, SendTimeoutOptionFailureDoesNotBindOrStartTask) {
    options_fail_at=2; EXPECT_EQ(dns_server_start(),ESP_FAIL); EXPECT_EQ(close_calls,1);
    EXPECT_EQ(bind_calls,0); EXPECT_EQ(task_calls,0);
}
TEST_F(ReadPicoDnsServer, BindFailureCannotBeReportedAsSuccessfulStartup) {
    fail_bind=true; EXPECT_EQ(dns_server_start(),ESP_FAIL); EXPECT_EQ(close_calls,1); EXPECT_EQ(task_calls,0);
    fail_bind=false; EXPECT_EQ(dns_server_start(),ESP_OK);
}
TEST_F(ReadPicoDnsServer, TaskCreationFailureLeavesSocketWithCallerForOneClose) {
    fail_task=true; EXPECT_EQ(dns_server_start(),ESP_FAIL); EXPECT_EQ(close_calls,1); EXPECT_EQ(delete_calls,0);
    fail_task=false; EXPECT_EQ(dns_server_start(),ESP_OK);
}
TEST_F(ReadPicoDnsServer, StartCleanupCloseFailureBlocksRestartAndRepeatedClose) {
    fail_task=fail_close=true; EXPECT_EQ(dns_server_start(),ESP_FAIL);
    fail_task=fail_close=false; EXPECT_EQ(dns_server_start(),ESP_ERR_INVALID_STATE);
    EXPECT_EQ(dns_server_stop_checked(),ESP_FAIL); EXPECT_EQ(close_calls,1); EXPECT_EQ(socket_calls,1);
}
TEST_F(ReadPicoDnsServer, StopTimeoutRetainsWorkerOwnershipUntilLaterExplicitAcknowledgement) {
    ASSERT_EQ(dns_server_start(),ESP_OK); defer_wait=true;
    EXPECT_EQ(dns_server_stop_checked(),ESP_ERR_TIMEOUT); EXPECT_EQ(close_calls,0);
    EXPECT_EQ(dns_server_start(),ESP_ERR_INVALID_STATE); EXPECT_EQ(socket_calls,1);
    defer_wait=false; run_at_wait=true; EXPECT_EQ(dns_server_stop_checked(),ESP_OK);
    EXPECT_EQ(close_calls,1); EXPECT_EQ(dns_server_start(),ESP_OK);
}
TEST_F(ReadPicoDnsServer, WorkerCloseFailureNeverGetsDoubleClosedOrReused) {
    ASSERT_EQ(dns_server_start(),ESP_OK); fail_close=true; run_at_wait=true;
    EXPECT_EQ(dns_server_stop_checked(),ESP_FAIL); EXPECT_EQ(close_calls,1);
    fail_close=false; EXPECT_EQ(dns_server_stop_checked(),ESP_FAIL); EXPECT_EQ(close_calls,1);
    EXPECT_EQ(dns_server_start(),ESP_ERR_INVALID_STATE);
}
TEST_F(ReadPicoDnsServer, EarlyWorkerFailureCannotPublishRunningSuccess) {
    run_early=true; fail_recv=true; EXPECT_EQ(dns_server_start(),ESP_FAIL); EXPECT_EQ(close_calls,1);
    EXPECT_EQ(dns_server_start(),ESP_ERR_INVALID_STATE); EXPECT_EQ(dns_server_stop_checked(),ESP_OK);
    run_early=fail_recv=false; EXPECT_EQ(dns_server_start(),ESP_OK);
}
TEST_F(ReadPicoDnsServer, InvalidPacketsAndOversizedDatagramsNeverReachSend) {
    packets={{1,2,3},std::vector<uint8_t>(600,0),valid_query()};
    ASSERT_EQ(dns_server_start(),ESP_OK); run_worker(); EXPECT_EQ(send_calls,1); EXPECT_EQ(sent.size(),35u);
    EXPECT_EQ(close_calls,1); EXPECT_EQ(dns_server_start(),ESP_ERR_INVALID_STATE);
    EXPECT_EQ(dns_server_stop_checked(),ESP_OK);
}
TEST_F(ReadPicoDnsServer, ReceiveAndSendTransientErrorsDoNotPrematurelyCloseWorker) {
    transient_recv=2; transient_send=1; packets={valid_query(),valid_query()};
    ASSERT_EQ(dns_server_start(),ESP_OK); run_worker(); EXPECT_EQ(send_calls,2); EXPECT_EQ(sent.size(),35u);
    EXPECT_EQ(close_calls,1); EXPECT_EQ(delete_calls,1);
}
TEST_F(ReadPicoDnsServer, ShortSendEndsWorkerAndRequiresExplicitStopBeforeRestart) {
    short_send=true; packets={valid_query(),valid_query()};
    ASSERT_EQ(dns_server_start(),ESP_OK); run_worker(); EXPECT_EQ(send_calls,1); EXPECT_EQ(close_calls,1);
    EXPECT_EQ(dns_server_start(),ESP_ERR_INVALID_STATE); EXPECT_EQ(dns_server_stop_checked(),ESP_OK);
}
TEST_F(ReadPicoDnsServer, FatalSendEndsWorkerWithoutRepeatingResponse) {
    fail_send=true; packets={valid_query(),valid_query()};
    ASSERT_EQ(dns_server_start(),ESP_OK); run_worker(); EXPECT_EQ(send_calls,1); EXPECT_EQ(close_calls,1);
}
TEST_F(ReadPicoDnsServer, NestedStartCannotCreateDuplicateSocketOrWorker) {
    nested_start=true; ASSERT_EQ(dns_server_start(),ESP_OK); EXPECT_EQ(socket_calls,1); EXPECT_EQ(task_calls,1);
}
TEST_F(ReadPicoDnsServer, StopBeforeStartIsIdempotentAndDoesNotTouchSocketOrTask) {
    EXPECT_EQ(dns_server_stop_checked(),ESP_OK); EXPECT_EQ(dns_server_stop_checked(),ESP_OK);
    EXPECT_EQ(socket_calls,0); EXPECT_EQ(close_calls,0); EXPECT_EQ(delete_calls,0);
}
