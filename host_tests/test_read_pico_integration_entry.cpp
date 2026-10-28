#include <gtest/gtest.h>

extern "C" void app_main(void);
static int product_calls;
extern "C" void photoframe_app_main(void)
{
    ++product_calls;
}

#if CONFIG_READ_PICO_LOCAL_FRAME
TEST(ReadPicoLocalFrameEntry, StartupInvokesProductExactlyOnce)
#else
TEST(ReadPicoIntegrationEntry, StartupNeverInvokesRealProductOrHardware)
#endif
{
    product_calls = 0;
    app_main();
#if CONFIG_READ_PICO_LOCAL_FRAME
    EXPECT_EQ(product_calls, 1);
#else
    EXPECT_EQ(product_calls, 0);
#endif
}
