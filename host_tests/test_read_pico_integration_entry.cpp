#include <gtest/gtest.h>

extern "C" void app_main(void);
static int product_calls;
extern "C" void photoframe_app_main(void)
{
    ++product_calls;
}

TEST(ReadPicoIntegrationEntry, StartupNeverInvokesRealProductOrHardware)
{
    product_calls = 0;
    app_main();
    EXPECT_EQ(product_calls, 0);
}
