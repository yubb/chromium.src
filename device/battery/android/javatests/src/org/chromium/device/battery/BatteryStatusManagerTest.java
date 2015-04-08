// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.battery;

import android.content.Intent;
import android.os.BatteryManager;
import android.os.Build;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import org.chromium.mojom.device.BatteryStatus;

/**
 * Test suite for BatteryStatusManager.
 */
public class BatteryStatusManagerTest extends AndroidTestCase {
    // Values reported in the most recent callback from |mManager|.
    private boolean mCharging = false;
    private double mChargingTime = 0;
    private double mDischargingTime = 0;
    private double mLevel = 0;

    private BatteryStatusManager.BatteryStatusCallback mCallback =
            new BatteryStatusManager.BatteryStatusCallback() {
        @Override
        public void onBatteryStatusChanged(BatteryStatus batteryStatus) {
            mCharging = batteryStatus.charging;
            mChargingTime = batteryStatus.chargingTime;
            mDischargingTime = batteryStatus.dischargingTime;
            mLevel = batteryStatus.level;
        }
    };

    private BatteryStatusManager mManager;

    // Can be non-null only for versions L and higher.
    private BatteryManagerForTesting mLollipopManager;

    private void verifyValues(
            boolean charging, double chargingTime, double dischargingTime, double level) {
        assertEquals(charging, mCharging);
        assertEquals(chargingTime, mChargingTime);
        assertEquals(dischargingTime, mDischargingTime);
        assertEquals(level, mLevel);
    }

    private static class BatteryManagerForTesting extends BatteryManager {
        private int mChargeCounter;
        private int mCapacity;
        private int mAverageCurrent;

        @Override
        public int getIntProperty(int id) {
            switch(id) {
                case BatteryManager.BATTERY_PROPERTY_CHARGE_COUNTER :
                    return mChargeCounter;
                case BatteryManager.BATTERY_PROPERTY_CAPACITY:
                    return mCapacity;
                case BatteryManager.BATTERY_PROPERTY_CURRENT_AVERAGE:
                    return mAverageCurrent;
            }
            fail();
            return 0;
        }

        public void setIntProperty(int id, int value) {
            switch(id) {
                case BatteryManager.BATTERY_PROPERTY_CHARGE_COUNTER :
                    mChargeCounter = value;
                    return;
                case BatteryManager.BATTERY_PROPERTY_CAPACITY:
                    mCapacity = value;
                    return;
                case BatteryManager.BATTERY_PROPERTY_CURRENT_AVERAGE:
                    mAverageCurrent = value;
                    return;
            }
            fail();
        }
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        initializeManager(null);
    }

    public void initializeManager(BatteryManagerForTesting managerForTesting) {
        mLollipopManager = managerForTesting;
        mManager = BatteryStatusManager.createBatteryStatusManagerForTesting(
                getContext(), mCallback, managerForTesting);
    }

    @SmallTest
    public void testOnReceiveBatteryNotPluggedIn() {
        Intent intent = new Intent(Intent.ACTION_BATTERY_CHANGED);
        intent.putExtra(BatteryManager.EXTRA_PRESENT, true);
        intent.putExtra(BatteryManager.EXTRA_PLUGGED, 0);
        intent.putExtra(BatteryManager.EXTRA_LEVEL, 10);
        intent.putExtra(BatteryManager.EXTRA_SCALE, 100);
        intent.putExtra(BatteryManager.EXTRA_STATUS, BatteryManager.BATTERY_STATUS_NOT_CHARGING);

        mManager.onReceive(intent);
        verifyValues(false, Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY, 0.1);
    }

    @SmallTest
    public void testOnReceiveBatteryPluggedInACCharging() {
        Intent intent = new Intent(Intent.ACTION_BATTERY_CHANGED);
        intent.putExtra(BatteryManager.EXTRA_PRESENT, true);
        intent.putExtra(BatteryManager.EXTRA_PLUGGED, BatteryManager.BATTERY_PLUGGED_AC);
        intent.putExtra(BatteryManager.EXTRA_LEVEL, 50);
        intent.putExtra(BatteryManager.EXTRA_SCALE, 100);
        intent.putExtra(BatteryManager.EXTRA_STATUS, BatteryManager.BATTERY_STATUS_CHARGING);

        mManager.onReceive(intent);
        verifyValues(true, Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY, 0.5);
    }

    @SmallTest
    public void testOnReceiveBatteryPluggedInACNotCharging() {
        Intent intent = new Intent(Intent.ACTION_BATTERY_CHANGED);
        intent.putExtra(BatteryManager.EXTRA_PRESENT, true);
        intent.putExtra(BatteryManager.EXTRA_PLUGGED, BatteryManager.BATTERY_PLUGGED_AC);
        intent.putExtra(BatteryManager.EXTRA_LEVEL, 50);
        intent.putExtra(BatteryManager.EXTRA_SCALE, 100);
        intent.putExtra(BatteryManager.EXTRA_STATUS, BatteryManager.BATTERY_STATUS_NOT_CHARGING);

        mManager.onReceive(intent);
        verifyValues(true, Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY, 0.5);
    }

    @SmallTest
    public void testOnReceiveBatteryPluggedInUSBFull() {
        Intent intent = new Intent(Intent.ACTION_BATTERY_CHANGED);
        intent.putExtra(BatteryManager.EXTRA_PRESENT, true);
        intent.putExtra(BatteryManager.EXTRA_PLUGGED, BatteryManager.BATTERY_PLUGGED_USB);
        intent.putExtra(BatteryManager.EXTRA_LEVEL, 100);
        intent.putExtra(BatteryManager.EXTRA_SCALE, 100);
        intent.putExtra(BatteryManager.EXTRA_STATUS, BatteryManager.BATTERY_STATUS_FULL);

        mManager.onReceive(intent);
        verifyValues(true, 0, Double.POSITIVE_INFINITY, 1);
    }

    @SmallTest
    public void testOnReceiveNoBattery() {
        Intent intent = new Intent(Intent.ACTION_BATTERY_CHANGED);
        intent.putExtra(BatteryManager.EXTRA_PRESENT, false);
        intent.putExtra(BatteryManager.EXTRA_PLUGGED, BatteryManager.BATTERY_PLUGGED_USB);

        mManager.onReceive(intent);
        verifyValues(true, 0, Double.POSITIVE_INFINITY, 1);
    }

    @SmallTest
    public void testOnReceiveNoPluggedStatus() {
        Intent intent = new Intent(Intent.ACTION_BATTERY_CHANGED);
        intent.putExtra(BatteryManager.EXTRA_PRESENT, true);

        mManager.onReceive(intent);
        verifyValues(true, 0, Double.POSITIVE_INFINITY, 1);
    }

    @SmallTest
    public void testStartStopSucceeds() {
        assertTrue(mManager.start());
        mManager.stop();
    }

    @SmallTest
    public void testLollipopChargingTimeEstimate() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) return;

        initializeManager(new BatteryManagerForTesting());

        Intent intent = new Intent(Intent.ACTION_BATTERY_CHANGED);
        intent.putExtra(BatteryManager.EXTRA_PRESENT, true);
        intent.putExtra(BatteryManager.EXTRA_PLUGGED, BatteryManager.BATTERY_PLUGGED_USB);
        intent.putExtra(BatteryManager.EXTRA_LEVEL, 50);
        intent.putExtra(BatteryManager.EXTRA_SCALE, 100);

        mLollipopManager.setIntProperty(BatteryManager.BATTERY_PROPERTY_CHARGE_COUNTER, 1000);
        mLollipopManager.setIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY, 50);
        mLollipopManager.setIntProperty(BatteryManager.BATTERY_PROPERTY_CURRENT_AVERAGE, 100);

        mManager.onReceive(intent);
        verifyValues(true, 0.5 * 10 * 3600, Double.POSITIVE_INFINITY, 0.5);
    }

    @SmallTest
    public void testLollipopDischargingTimeEstimate() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) return;

        initializeManager(new BatteryManagerForTesting());

        Intent intent = new Intent(Intent.ACTION_BATTERY_CHANGED);
        intent.putExtra(BatteryManager.EXTRA_PRESENT, true);
        intent.putExtra(BatteryManager.EXTRA_PLUGGED, 0);
        intent.putExtra(BatteryManager.EXTRA_LEVEL, 60);
        intent.putExtra(BatteryManager.EXTRA_SCALE, 100);
        intent.putExtra(BatteryManager.EXTRA_STATUS, BatteryManager.BATTERY_STATUS_NOT_CHARGING);

        mLollipopManager.setIntProperty(BatteryManager.BATTERY_PROPERTY_CHARGE_COUNTER, 1000);
        mLollipopManager.setIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY, 60);
        mLollipopManager.setIntProperty(BatteryManager.BATTERY_PROPERTY_CURRENT_AVERAGE, -100);

        mManager.onReceive(intent);
        verifyValues(false, Double.POSITIVE_INFINITY, 0.6 * 10 * 3600, 0.6);
    }

    @SmallTest
    public void testLollipopDischargingTimeEstimateRounding() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) return;

        initializeManager(new BatteryManagerForTesting());

        Intent intent = new Intent(Intent.ACTION_BATTERY_CHANGED);
        intent.putExtra(BatteryManager.EXTRA_PRESENT, true);
        intent.putExtra(BatteryManager.EXTRA_PLUGGED, 0);
        intent.putExtra(BatteryManager.EXTRA_LEVEL, 90);
        intent.putExtra(BatteryManager.EXTRA_SCALE, 100);
        intent.putExtra(BatteryManager.EXTRA_STATUS, BatteryManager.BATTERY_STATUS_NOT_CHARGING);

        mLollipopManager.setIntProperty(BatteryManager.BATTERY_PROPERTY_CHARGE_COUNTER, 1999);
        mLollipopManager.setIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY, 90);
        mLollipopManager.setIntProperty(BatteryManager.BATTERY_PROPERTY_CURRENT_AVERAGE, -1000);

        mManager.onReceive(intent);
        verifyValues(false, Double.POSITIVE_INFINITY, Math.floor(0.9 * 1.999 * 3600), 0.9);
    }
}
