package com.apiguard.apiary.data.local

import androidx.room.*
import com.apiguard.apiary.data.model.ApiaryReading
import kotlinx.coroutines.flow.Flow

@Dao
interface ApiaryDao {
    @Query("SELECT * FROM apiary_readings WHERE apiaryId = :apiaryId ORDER BY timestamp DESC")
    fun getReadingsByApiary(apiaryId: String): Flow<List<ApiaryReading>>
    
    @Query("SELECT * FROM apiary_readings ORDER BY timestamp DESC")
    fun getAllReadings(): Flow<List<ApiaryReading>>
    
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertReading(reading: ApiaryReading)
    
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertReadings(readings: List<ApiaryReading>)
    
    @Query("DELETE FROM apiary_readings WHERE apiaryId = :apiaryId")
    suspend fun deleteReadingsByApiary(apiaryId: String)
    
    @Query("DELETE FROM apiary_readings")
    suspend fun deleteAllReadings()
    
    @Query("SELECT MAX(timestamp) FROM apiary_readings WHERE apiaryId = :apiaryId")
    suspend fun getLastTimestampForApiary(apiaryId: String): Long?
}
