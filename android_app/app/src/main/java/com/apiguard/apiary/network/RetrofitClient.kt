package com.apiguard.apiary.network

import com.apiguard.apiary.data.remote.ApiaryApiService

import okhttp3.OkHttpClient
import okhttp3.logging.HttpLoggingInterceptor
import retrofit2.Retrofit
import retrofit2.converter.gson.GsonConverterFactory
import java.util.concurrent.TimeUnit

object RetrofitClient {
    private const val TIMEOUT_SECONDS = 10L

    fun buildBaseUrl(host: String, port: Int): String {
        require(host.isNotBlank()) { "Host nie może być pusty" }
        require(port in 1..65535) { "Port musi należeć do zakresu 1..65535" }
        return "https://$host:$port/"
    }
    
    fun createApiService(baseUrl: String): ApiaryApiService {
        val loggingInterceptor = HttpLoggingInterceptor().apply {
            level = if (com.apiguard.apiary.BuildConfig.DEBUG) HttpLoggingInterceptor.Level.BASIC else HttpLoggingInterceptor.Level.NONE
        }
        
        val okHttpClient = OkHttpClient.Builder()
            .addInterceptor(loggingInterceptor)
            .connectTimeout(TIMEOUT_SECONDS, TimeUnit.SECONDS)
            .readTimeout(TIMEOUT_SECONDS, TimeUnit.SECONDS)
            .writeTimeout(TIMEOUT_SECONDS, TimeUnit.SECONDS)
            .build()
        
        val retrofit = Retrofit.Builder()
            .baseUrl(baseUrl)
            .client(okHttpClient)
            .addConverterFactory(GsonConverterFactory.create())
            .build()
        
        return retrofit.create(ApiaryApiService::class.java)
    }
}
