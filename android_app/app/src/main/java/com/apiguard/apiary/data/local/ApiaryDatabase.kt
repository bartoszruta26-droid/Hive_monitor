package com.apiguard.apiary.data.local

import android.content.Context
import androidx.room.Database
import androidx.room.Room
import androidx.room.RoomDatabase
import com.apiguard.apiary.data.model.ApiaryReading

@Database(entities = [ApiaryReading::class], version = 1, exportSchema = false)
abstract class ApiaryDatabase : RoomDatabase() {
    abstract fun apiaryDao(): ApiaryDao
    
    companion object {
        @Volatile
        private var INSTANCE: ApiaryDatabase? = null
        
        fun getDatabase(context: Context): ApiaryDatabase {
            return INSTANCE ?: synchronized(this) {
                val instance = Room.databaseBuilder(
                    context.applicationContext,
                    ApiaryDatabase::class.java,
                    "apiary_database"
                )
                .fallbackToDestructiveMigration()
                .build()
                INSTANCE = instance
                instance
            }
        }
    }
}
