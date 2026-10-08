#include <gtest/gtest.h>
#include <array>
#include <vector>
#include <cstring>
#include "captive_dns.h"
namespace {
using Bytes = std::vector<uint8_t>;
Bytes query(uint16_t type = 1) {
    return {0x12,0x34,1,0,0,1,0,0,0,0,0,0,7,'e','x','a','m','p','l','e',4,'t','e','s','t',0,
            (uint8_t)(type >> 8),(uint8_t)type,0,1};
}
void opt(Bytes &q, const Bytes &data = {}, uint8_t version = 0) {
    q[11] = 1;
    Bytes record{0,0,41,0x10,0,0,version,0x80,0, (uint8_t)(data.size()>>8), (uint8_t)data.size()};
    q.insert(q.end(), record.begin(), record.end()); q.insert(q.end(), data.begin(), data.end());
}
const uint8_t ip[4] = {192,168,4,1};
bool reply(const Bytes &q, uint8_t *out, size_t cap, size_t *size) {
    return captive_dns_response(q.data(), q.size(), ip, out, cap, size);
}
void rejected(const Bytes &q) {
    std::array<uint8_t,512> out; out.fill(0x5a); auto before = out; size_t size = 777;
    EXPECT_FALSE(reply(q, out.data(), out.size(), &size)); EXPECT_EQ(out, before); EXPECT_EQ(size, 777u);
}
}
TEST(CaptiveDns, ExactAResponseHasCheckedQuestionAddressAndNoForgedFlags) {
    auto q = query(); q[3] = 0x30; std::array<uint8_t,512> out{}; size_t size = 0;
    ASSERT_TRUE(reply(q, out.data(), out.size(), &size)); EXPECT_EQ(size, q.size()+16);
    EXPECT_EQ(out[0], 0x12); EXPECT_EQ(out[1], 0x34); EXPECT_EQ(out[2], 0x81); EXPECT_EQ(out[3], 0);
    EXPECT_EQ(out[7], 1); EXPECT_EQ(out[11], 0); EXPECT_EQ(out[q.size()], 0xc0);
    EXPECT_EQ(memcmp(out.data()+size-4, ip, 4), 0);
    EXPECT_EQ(memcmp(out.data()+12, q.data()+12, q.size()-12), 0);
}
TEST(CaptiveDns, NonAQuestionsNeverReceiveMismatchedAAnswers) {
    for (auto type : {28, 16, 255, 5}) {
        auto q = query(type); std::array<uint8_t,512> out{}; size_t size = 0;
        ASSERT_TRUE(reply(q, out.data(), out.size(), &size)); EXPECT_EQ(size, q.size()); EXPECT_EQ(out[7], 0);
    }
}
TEST(CaptiveDns, RootNameAndNoRecursionDesiredRemainValid) {
    Bytes q{0,1,0,0,0,1,0,0,0,0,0,0,0,0,1,0,1};
    std::array<uint8_t,512> out{}; size_t size = 0;
    ASSERT_TRUE(reply(q, out.data(), out.size(), &size)); EXPECT_EQ(out[2],0x80); EXPECT_EQ(size,33u);
}
TEST(CaptiveDns, EveryTruncatedPrefixAndUnclaimedSuffixIsRejected) {
    auto q = query(); for (size_t n=0;n<q.size();++n) rejected(Bytes(q.begin(),q.begin()+n));
    q.push_back(0); rejected(q);
}
TEST(CaptiveDns, HeaderCountsFlagsClassAndCompressedQuestionAreRejected) {
    for (int pos : {2,4,5,6,8,10,11,29}) {
        auto q = query(); q[pos] = 2; rejected(q);
    }
    auto q = query(); q[2] = 0x81; rejected(q);
    q = query(); q[12] = 0xc0; q[13] = 12; rejected(q);
    q = query(); q[12] = 64; rejected(q);
}
TEST(CaptiveDns, NameWireLimitIsEnforcedIncludingTerminator) {
    auto q = query(); q.resize(12);
    for (int n : {63,63,63,61}) { q.push_back(n); q.insert(q.end(),n,'x'); }
    q.insert(q.end(),{0,0,1,0,1}); std::array<uint8_t,512> out{}; size_t size = 0;
    ASSERT_TRUE(reply(q,out.data(),out.size(),&size)); EXPECT_EQ(size,287u);
    q.insert(q.end()-5,'x'); q[204] = 62; rejected(q);
}
TEST(CaptiveDns, EdnsOptionsAreCheckedButNotCopiedIntoAnswerAndOptIsIncluded) {
    auto q=query(); opt(q,{0xfd,0xe8,0,3,1,2,3}); std::array<uint8_t,512> out{}; size_t size=0;
    ASSERT_TRUE(reply(q,out.data(),out.size(),&size)); EXPECT_EQ(size,57u); EXPECT_EQ(out[11],1);
    EXPECT_EQ(out[size-10],0); EXPECT_EQ(out[size-9],41); EXPECT_EQ(out[size-8],2);
    EXPECT_EQ(out[size-4],0x80); EXPECT_EQ(out[size-1],0);
}
TEST(CaptiveDns, UnsupportedEdnsVersionGetsBadversInsteadOfAnAAnswer) {
    auto q=query(); opt(q,{},1); std::array<uint8_t,512> out{}; size_t size=0;
    ASSERT_TRUE(reply(q,out.data(),out.size(),&size)); EXPECT_EQ(size,41u); EXPECT_EQ(out[7],0);
    EXPECT_EQ(out[size-6],1); EXPECT_EQ(out[size-5],0);
}
TEST(CaptiveDns, Full512ByteQueryCannotCauseAnswerAppendOverflow) {
    auto q=query(); Bytes data(471,0x5a); data[0]=0; data[1]=12; data[2]=1; data[3]=0xd3;
    opt(q,data); ASSERT_EQ(q.size(),512u);
    std::array<uint8_t,514> out; out.fill(0x5a); size_t size=0;
    ASSERT_TRUE(reply(q,out.data()+1,512,&size)); EXPECT_EQ(size,57u);
    EXPECT_EQ(out.front(),0x5a); EXPECT_EQ(out.back(),0x5a);
    q.push_back(0); rejected(q);
}
TEST(CaptiveDns, MalformedOptRecordsAndOptionLengthsPreserveOutput) {
    auto base=query(); opt(base,{1,2,0,1,9});
    for (size_t n=query().size();n<base.size();++n) rejected(Bytes(base.begin(),base.begin()+n));
    for (int pos : {30,32,35,40,44}) { auto q=base; q[pos]=0xff; rejected(q); }
    auto q=base; q[11]=2; rejected(q);
}
TEST(CaptiveDns, CapacityAndNullFailuresDoNotPublishPartialBytesOrLength) {
    auto q=query(); std::array<uint8_t,512> out; out.fill(0x5a); size_t size=777;
    EXPECT_FALSE(reply(q,out.data(),q.size()+15,&size)); EXPECT_EQ(size,777u); EXPECT_EQ(out[0],0x5a);
    EXPECT_FALSE(captive_dns_response(nullptr,q.size(),ip,out.data(),512,&size));
    EXPECT_FALSE(captive_dns_response(q.data(),q.size(),nullptr,out.data(),512,&size));
    EXPECT_FALSE(reply(q,nullptr,512,&size)); EXPECT_FALSE(reply(q,out.data(),512,nullptr));
}
TEST(CaptiveDns, DeterministicMalformedCorpusNeverWritesPastProvidedCapacity) {
    uint32_t seed=0x12345678;
    for (int round=0;round<10000;++round) {
        seed=seed*1664525+1013904223; Bytes q(seed%514);
        for (auto &c:q) { seed=seed*1664525+1013904223; c=seed>>24; }
        std::array<uint8_t,514> out; out.fill(0x5a); size_t size=777; size_t capacity=seed%513;
        bool ok=reply(q,out.data()+1,capacity,&size);
        EXPECT_EQ(out.front(),0x5a); EXPECT_EQ(out[capacity+1],0x5a);
        if (ok) EXPECT_LE(size,capacity); else EXPECT_EQ(size,777u);
    }
}
