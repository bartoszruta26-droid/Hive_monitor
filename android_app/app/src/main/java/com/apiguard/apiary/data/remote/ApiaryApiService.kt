package com.apiguard.apiary.data.remote

import com.google.gson.JsonObject
import retrofit2.Response
import retrofit2.http.GET
import retrofit2.http.Query

interface ApiaryApiService {
    @GET("health")
    suspend fun checkHealth(): Response<JsonObject>
    
    @GET("api/hives")
    suspend fun getApiaries(): Response<JsonObject>

    @GET("api/history")
    suspend fun getHistory(
        @Query("hive_id") hiveId: String,
        @Query("aggregation") aggregation: String,
        @Query("start") startTimestamp: Long,
        @Query("end") endTimestamp: Long,
        @Query("metric") metric: String? = null,
        @Query("include_records") includeRecords: Boolean = false
    ): Response<com.apiguard.apiary.data.model.ApiaryHistoryResponse>
}
