/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.systemui.qs.tiles;

import android.content.Intent;

import com.android.internal.logging.MetricsLogger;
import com.android.systemui.R;
import com.android.systemui.qs.QSTile;
import com.android.systemui.statusbar.policy.KeyguardMonitor;
import com.android.systemui.statusbar.policy.ScreenrecordController;

/** Quick settings tile: Screenrecord **/
public class ScreenrecordTile extends QSTile<QSTile.BooleanState>
                implements ScreenrecordController.Callback {
    private final ScreenrecordController mScreenrecord;
    private final KeyguardMonitor mKeyguard;
    private final Callback mCallback = new Callback();

    public ScreenrecordTile(Host host) {
        super(host);
        mScreenrecord = host.getScreenrecordController();
        mKeyguard = host.getKeyguardMonitor();
    }

    @Override
    public BooleanState newTileState() {
        return new BooleanState();
    }

    @Override
    public void setListening(boolean listening) {
        if (listening) {
            mScreenrecord.addCallback(this);
            mKeyguard.addCallback(mCallback);
        } else {
            mScreenrecord.removeCallback(this);
            mKeyguard.removeCallback(mCallback);
        }
    }

    @Override
    protected void handleClick() {
        mHost.collapsePanels();
        mScreenrecord.autoRecord();
    }

	@Override	 
	public Intent getLongClickIntent() {
		//new feature
		return null;	
	}

    @Override
    protected void handleUpdateState(BooleanState state, Object arg) {
        //state.visible = !mKeyguard.isShowing();
        state.value = mScreenrecord.isRecording();
        state.label = mContext.getString(R.string.quick_settings_screenrecord_label);
        state.icon = ResourceIcon.get(state.value
            ? R.drawable.ic_qs_screenrecord_recording : R.drawable.ic_qs_screenrecord_stop);
    }

    private final class Callback implements KeyguardMonitor.Callback {
        @Override
        public void onKeyguardChanged() {
            refreshState();
        }
    };

    @Override
    public void onStateChange(boolean isRecording) {
        refreshState();
    }

    @Override
    public int getMetricsCategory() {
        return MetricsLogger.QS_SCREENRECORD;
    }

	@Override	 
	public CharSequence getTileLabel() {		
		return mContext.getString(R.string.quick_settings_screenrecord_label);	
	}
	
}
