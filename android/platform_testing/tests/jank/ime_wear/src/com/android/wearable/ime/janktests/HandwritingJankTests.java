/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.wearable.ime.janktests;

import android.os.Bundle;
import android.support.test.jank.JankTest;
import android.support.test.jank.JankTestBase;
import android.support.test.jank.WindowAnimationFrameStatsMonitor;
import android.support.test.uiautomator.UiDevice;

/**
 * Jank tests for handwriting on wear
 */
public class HandwritingJankTests extends JankTestBase {

    private UiDevice mDevice;
    private IMEJankTestsHelper mHelper;

    /*
     * (non-Javadoc)
     * @see junit.framework.TestCase#setUp()
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(getInstrumentation());
        mHelper = IMEJankTestsHelper.getInstance(mDevice, this.getInstrumentation());
        mDevice.wakeUp();
        mHelper.activateIMEHandwriting();
    }

    public void launchRemoteInputActivity() {
        mHelper.goBackHome();
        mHelper.launchRemoteInputActivity();
    }

    public void launchInputBoxActivity() {
        mHelper.goBackHome();
        mHelper.launchInputBoxActivity();
    }

    // Measure handwriting jank when open from remote input
    @JankTest(beforeTest = "launchRemoteInputActivity",
            afterLoop = "pressBack",
            afterTest = "goBackHome",
            expectedFrames = IMEJankTestsHelper.WFM_EXPECTED_FRAMES)
    @WindowAnimationFrameStatsMonitor
    public void testOpenHandwritingFromRemoteInput() {
        mHelper.tapIMEButton();
    }

    // Measure handwriting jank when open from input box
    @JankTest(beforeTest = "launchInputBoxActivity",
            afterLoop = "pressBack",
            afterTest = "goBackHome",
            expectedFrames = IMEJankTestsHelper.WFM_EXPECTED_FRAMES)
    @WindowAnimationFrameStatsMonitor
    public void testOpenHandwritingFromInputBox() {
        mHelper.tapOnScreen();
    }

    public void pressBack() {
        mHelper.pressBack();
    }

    // Ensuring that we head back to the first screen before launching the app again
    public void goBackHome(Bundle metrics) {
        mHelper.goBackHome();
        super.afterTest(metrics);
    }

}