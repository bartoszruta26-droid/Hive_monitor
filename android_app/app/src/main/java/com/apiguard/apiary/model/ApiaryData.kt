package com.apiguard.apiary.model

import com.google.gson.annotations.SerializedName

data class ApiaryData(
    @SerializedName("id") val id: String,
    @SerializedName("name") val name: String,
    @SerializedName("temperature") val temperature: Double,
    @SerializedName("humidity") val humidity: Double,
    @SerializedName("weight") val weight: Double,
    @SerializedName("battery") val battery: Int,
    @SerializedName("timestamp") val timestamp: Long
)
