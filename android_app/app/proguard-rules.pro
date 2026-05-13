# Add project specific ProGuard rules here.
-keepattributes Signature
-keepattributes *Annotation*
-keep class com.apiguard.apiary.model.** { *; }
-dontwarn retrofit2.**
-keep class retrofit2.** { *; }
-keepattributes Exceptions
