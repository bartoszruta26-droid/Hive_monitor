package com.apiguard.apiary.data.remote

import com.google.gson.JsonObject
import retrofit2.Response
import retrofit2.http.GET
import retrofit2.http.Path
import retrofit2.http.Query

interface ApiaryApiService {
    @GET("health")
    suspend fun checkHealth(): Response<Unit>
    
    @GET("api/hives")
    suspend fun getApiaries(): Response<JsonObject>
    
    @GET("api/apiaries/{id}")
    suspend fun getApiaryById(@Path("id") apiaryId: String): Response<ApiaryData>
    
    @GET("api/apiaries/{id}/history")
    suspend fun getApiaryHistory(
        @Path("id") apiaryId: String,
        @Query("since") sinceTimestamp: Long? = null
    ): Response<List<ApiaryData>>
}
