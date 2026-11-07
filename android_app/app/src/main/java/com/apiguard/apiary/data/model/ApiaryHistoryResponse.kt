package com.apiguard.apiary.data.model

import com.google.gson.annotations.SerializedName

data class ApiaryHistoryResponse(
    @SerializedName("hive_id") val hiveId: String,
    val aggregation: String,
    val start: Long,
    val end: Long,
    @SerializedName("data_quality") val dataQuality: String,
    val records: List<ApiaryHistoryRecord>
)

data class ApiaryHistoryRecord(
    @SerializedName("timestamp_start") val timestampStart: Long,
    @SerializedName("timestamp_end") val timestampEnd: Long,
    @SerializedName("temperature_avg") val temperatureAverage: Double,
    @SerializedName("temperature_min") val temperatureMinimum: Double,
    @SerializedName("temperature_max") val temperatureMaximum: Double,
    @SerializedName("humidity_avg") val humidityAverage: Double,
    @SerializedName("humidity_min") val humidityMinimum: Double,
    @SerializedName("humidity_max") val humidityMaximum: Double,
    @SerializedName("weight_avg") val weightAverage: Double,
    @SerializedName("weight_min") val weightMinimum: Double,
    @SerializedName("weight_max") val weightMaximum: Double,
    @SerializedName("battery_avg") val batteryAverage: Double,
    @SerializedName("co2_avg") val co2Average: Double,
    @SerializedName("voc_avg") val vocAverage: Double,
    @SerializedName("audio_rms_avg") val audioRmsAverage: Double,
    @SerializedName("audio_dominant_freq_avg") val audioDominantFrequencyAverage: Double,
    @SerializedName("audio_swarm_prob_avg") val audioSwarmProbabilityAverage: Double,
    @SerializedName("audio_bee_activity_avg") val audioBeeActivityAverage: Double,
    @SerializedName("radar_distance_avg") val radarDistanceAverage: Double,
    @SerializedName("radar_energy_avg") val radarEnergyAverage: Double,
    @SerializedName("radar_activity_avg") val radarActivityAverage: Double,
    @SerializedName("hx711_current_avg") val hx711CurrentAverage: Double,
    @SerializedName("hx711_slope_24h_avg") val hx711Slope24hAverage: Double,
    @SerializedName("record_count") val recordCount: Long
)
