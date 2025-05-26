package com.example.myblegpsapp

import android.Manifest
import android.bluetooth.BluetoothAdapter
import android.bluetooth.le.AdvertiseCallback
import android.bluetooth.le.AdvertiseData
import android.bluetooth.le.AdvertiseSettings
import android.bluetooth.le.BluetoothLeAdvertiser
import android.os.Bundle
import android.os.Looper
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import com.google.android.gms.location.*
import java.nio.ByteBuffer
import java.nio.ByteOrder

class MainActivity : AppCompatActivity() {
    private lateinit var advertiser: BluetoothLeAdvertiser
    private lateinit var fusedLoc: FusedLocationProviderClient
    private lateinit var locCallback: LocationCallback
    private val advCallback = object : AdvertiseCallback() {
        override fun onStartSuccess(settingsInEffect: AdvertiseSettings) {
            Log.i("BLE", "Advertising started")
        }
        override fun onStartFailure(errorCode: Int) {
            Log.e("BLE", "Advertising failed: $errorCode")
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Request permissions
        val perms = arrayOf(
            Manifest.permission.ACCESS_FINE_LOCATION,
            Manifest.permission.ACCESS_COARSE_LOCATION,
            Manifest.permission.BLUETOOTH_ADVERTISE,
            Manifest.permission.BLUETOOTH_CONNECT
        )
        ActivityCompat.requestPermissions(this, perms, 1)

        val btAdapter = BluetoothAdapter.getDefaultAdapter()
        require(btAdapter.isMultipleAdvertisementSupported) {
            "BLE advertising not supported"
        }
        advertiser = btAdapter.bluetoothLeAdvertiser

        fusedLoc = LocationServices.getFusedLocationProviderClient(this)
        val req = LocationRequest.Builder(Priority.PRIORITY_HIGH_ACCURACY, 5000L).build()
        locCallback = object : LocationCallback() {
            override fun onLocationResult(result: LocationResult) {
                result.lastLocation?.let {
                    advertiseLocation(it.latitude, it.longitude)
                }
            }
        }
        fusedLoc.requestLocationUpdates(req, locCallback, Looper.getMainLooper())
    }

    private fun advertiseLocation(lat: Double, lon: Double) {
        advertiser.stopAdvertising(advCallback)
        val buf = ByteBuffer.allocate(8)
            .order(ByteOrder.LITTLE_ENDIAN)
            .putInt((lat * 1e6).toInt())
            .putInt((lon * 1e6).toInt())
            .array()

        val settings = AdvertiseSettings.Builder()
            .setAdvertiseMode(AdvertiseSettings.ADVERTISE_MODE_BALANCED)
            .setTxPowerLevel(AdvertiseSettings.ADVERTISE_TX_POWER_MEDIUM)
            .setConnectable(false)
            .build()

        val data = AdvertiseData.Builder()
            .addManufacturerData(0xFFFF, buf)
            .build()

        advertiser.startAdvertising(settings, data, advCallback)
    }

    override fun onDestroy() {
        advertiser.stopAdvertising(advCallback)
        fusedLoc.removeLocationUpdates(locCallback)
        super.onDestroy()
    }
}
