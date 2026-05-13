package com.apiguard.apiary.data.model

import androidx.room.Entity
import androidx.room.PrimaryKey

@Entity(tableName = "apiary_readings")
data class ApiaryReading(
    @PrimaryKey(autoGenerate = true)
    val id: Long = 0,
    val apiaryId: String,
    val timestamp: Long,
    val temperature: Float,
    val humidity: Float,
    val weight: Float,
    val batteryLevel: Int,
    val isCached: Boolean = true
)
