#include <gtest/gtest.h>
#include <thread>
#include "provisioning_session.h"
TEST(ProvisioningSession, OldStoredCredentialsDoNotCompleteANewSession) {
    auto ticket = provisioning_session_begin(); ASSERT_NE(ticket, 0u);
    EXPECT_FALSE(provisioning_session_complete());
    EXPECT_EQ(provisioning_session_ticket(), ticket);
    EXPECT_TRUE(provisioning_session_commit(ticket)); EXPECT_TRUE(provisioning_session_complete());
    EXPECT_EQ(provisioning_session_ticket(), 0u);
}
TEST(ProvisioningSession, StaleTicketAndDuplicateSaveCannotClaimCurrentSession) {
    auto old = provisioning_session_begin(); auto current = provisioning_session_begin();
    EXPECT_FALSE(provisioning_session_commit(old)); EXPECT_FALSE(provisioning_session_complete());
    EXPECT_TRUE(provisioning_session_commit(current)); EXPECT_FALSE(provisioning_session_commit(current));
}
TEST(ProvisioningSession, InvalidTicketAndUncommittedSaveStayIncomplete) {
    auto current = provisioning_session_begin();
    EXPECT_FALSE(provisioning_session_commit(0)); EXPECT_FALSE(provisioning_session_commit(current | 1));
    EXPECT_FALSE(provisioning_session_complete());
}
TEST(ProvisioningSession, NewSessionInvalidatesAnEarlierCompletedSave) {
    auto old = provisioning_session_begin(); ASSERT_TRUE(provisioning_session_commit(old));
    auto current = provisioning_session_begin(); EXPECT_NE(current, old);
    EXPECT_FALSE(provisioning_session_complete()); EXPECT_FALSE(provisioning_session_commit(old));
    EXPECT_TRUE(provisioning_session_commit(current));
}
TEST(ProvisioningSession, CancelledSessionCannotSaveOrCompleteAndNextTicketIsDifferent) {
    auto old = provisioning_session_begin(); ASSERT_NE(old, 0u);
    provisioning_session_cancel(); provisioning_session_cancel();
    EXPECT_EQ(provisioning_session_ticket(), 0u);
    EXPECT_FALSE(provisioning_session_complete());
    EXPECT_FALSE(provisioning_session_commit(old));
    EXPECT_FALSE(provisioning_session_commit(old | 2u));
    auto next = provisioning_session_begin(); ASSERT_NE(next, old);
    EXPECT_FALSE(provisioning_session_commit(old));
    EXPECT_TRUE(provisioning_session_commit(next));
}
TEST(ProvisioningSession, StopCancelsCompletedStatusRatherThanClaimingAnotherSessionSaved) {
    auto old = provisioning_session_begin(); ASSERT_TRUE(provisioning_session_commit(old));
    provisioning_session_cancel();
    EXPECT_FALSE(provisioning_session_complete());
    EXPECT_EQ(provisioning_session_ticket(), 0u);
    EXPECT_FALSE(provisioning_session_commit(old));
}
TEST(ProvisioningSession, ConcurrentCommitCannotResurrectSessionAfterCancel) {
    for (unsigned i = 0; i != 1000; ++i) {
        auto ticket = provisioning_session_begin(); ASSERT_NE(ticket, 0u);
        std::thread complete([ticket] { provisioning_session_commit(ticket); });
        provisioning_session_cancel(); complete.join();
        EXPECT_EQ(provisioning_session_ticket(), 0u);
        EXPECT_FALSE(provisioning_session_complete());
        EXPECT_FALSE(provisioning_session_commit(ticket));
    }
}
