package com.apiguard.apiary.data.remote

import com.apiguard.apiary.model.ApiaryData
import retrofit2.Response
import retrofit2.http.GET
import retrofit2.http.Path
import retrofit2.http.Query

interface ApiaryApiService {
    @GET("api/health")
    suspend fun checkHealth(): Response<Unit>
    
    @GET("api/apiaries")
    suspend fun getApiaries(): Response<List<ApiaryData>>
    
    @GET("api/apiaries/{id}")
    suspend fun getApiaryById(@Path("id") apiaryId: String): Response<ApiaryData>
    
    @GET("api/apiaries/{id}/history")
    suspend fun getApiaryHistory(
        @Path("id") apiaryId: String,
        @Query("since") sinceTimestamp: Long? = null
    ): Response<List<ApiaryData>>
}
