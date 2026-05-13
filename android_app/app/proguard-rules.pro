# Add project specific ProGuard rules here.

# Gson - zachowaj atrybuty i adnotacje
-keepattributes Signature
-keepattributes *Annotation*
-keepattributes EnclosingMethod
-keepattributes InnerClasses

# Modele danych - zachowaj dla serializacji/deserializacji
-keep class com.apiguard.apiary.model.** { *; }
-keep class com.apiguard.apiary.data.model.** { *; }

# Retrofit - obsługa błędów i wyjątków
-dontwarn retrofit2.**
-keep class retrofit2.** { *; }
-keepattributes Exceptions

# OkHttp - biblioteka używana przez Retrofit
-dontwarn okhttp3.**
-dontwarn okio.**
-keep class okhttp3.** { *; }
-keep interface okhttp3.** { *; }
-keep class okio.** { *; }
-keep interface okio.** { *; }

# Room - baza danych
-keep class * extends androidx.room.RoomDatabase
-keep @androidx.room.Entity class *
-dontwarn androidx.room.paging.**

# Kotlin Coroutines
-keepnames class kotlinx.coroutines.internal.MainDispatcherFactory {}
-keepnames class kotlinx.coroutines.CoroutineExceptionHandler {}
-keepclassmembers class kotlinx.coroutines.** { volatile <fields>; }

# Kotlin - zachowaj metadata
-keep class kotlin.Metadata { *; }
-keepclassmembers class **$WhenMappings { <fields>; }

# Życie aplikacji i ViewModel
-keep class * extends androidx.lifecycle.LiveData { *; }
-keep class * extends androidx.lifecycle.ViewModel { *; }

# RecyclerView i Adaptery
-keep class * extends androidx.recyclerview.widget.RecyclerView.Adapter { *; }

# Wyjątki - zachowaj informacje o błędach dla lepszego debugowania
-keepattributes SourceFile,LineNumberTable
