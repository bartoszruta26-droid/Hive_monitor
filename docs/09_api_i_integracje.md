## 🔌 API i Integracje

### REST API Endpoints

#### Audio Analysis Endpoints (MEMS Microphone)

```http
GET /audio/status
Authorization: Bearer {token}

Response:
{
  "timestamp": "2024-01-15T10:30:00Z",
  "firmware_version": "2.5.0",
  "uptime_seconds": 86400,
  "microphone_connected": true,
  "buffer_size": 256,
  "samples_collected": 4521,
  "current_metrics": {
    "rms_amplitude": 0.0234,
    "peak_amplitude": 0.0891,
    "zero_crossing_rate": 145.2,
    "dominant_frequency": 287.5,
    "spectral_centroid": 412.3,
    "bee_activity_index": 78.5,
    "swarm_probability": 12.3,
    "stress_indicator": 23.1,
    "hive_health_audio": 85.7
  }
}
```

```http
GET /audio/metrics
Authorization: Bearer {token}

Response:
{
  "timestamp": "2024-01-15T10:30:00Z",
  "time_domain": {
    "rms_amplitude": 0.0234,
    "peak_amplitude": 0.0891,
    "peak_to_peak": 0.1523,
    "zero_crossing_rate": 145.2,
    "signal_energy": 1247.5,
    "crest_factor": 3.81,
    "average_amplitude": 0.0187
  },
  "statistics": {
    "mean_value": 0.0012,
    "std_deviation": 0.0231,
    "skewness": -0.15,
    "kurtosis": 2.87,
    "coefficient_of_variation": 19.25,
    "dynamic_range": 42.3
  },
  "frequency_domain": {
    "dominant_frequency": 287.5,
    "spectral_centroid": 412.3,
    "spectral_bandwidth": 234.7,
    "spectral_flatness": 0.23,
    "spectral_rolloff": 678.9,
    "spectral_entropy": 4.56,
    "harmonic_to_noise_ratio": 12.34,
    "autocorrelation_peak": 0.78
  },
  "band_power": {
    "power_low_freq": 23.5,
    "power_bee_band": 567.8,
    "power_swarm_band": 234.5,
    "power_mid_freq": 123.4,
    "power_high_freq": 45.6
  },
  "classification": {
    "bee_activity_index": 78.5,
    "swarm_probability": 12.3,
    "stress_indicator": 23.1,
    "hive_health_audio": 85.7
  },
  "formants_quality": {
    "formant_f1": 245.3,
    "formant_f2": 567.8,
    "formant_f3": 1234.5,
    "brightness": 0.34,
    "roughness": 0.12,
    "sharpness": 0.45,
    "tonality": 0.67,
    "prominence_ratio": 2.34
  },
  "temporal_features": {
    "attack_time": 12.5,
    "decay_time": 45.3,
    "temporal_centroid": 78.9,
    "silence_ratio": 0.05,
    "modulation_index": 0.23
  },
  "psychoacoustics": {
    "loudness": 34.5,
    "roughness_fast": 0.15,
    "spectral_decrease": -0.23,
    "irregularity": 0.34
  }
}
```

```http
GET /audio/events
Authorization: Bearer {token}

Response:
{
  "timestamp": "2024-01-15T10:30:00Z",
  "status": "POZYTYWNY",
  "event_type": "INCREASED_ACTIVITY",
  "confidence": 0.92,
  "impact": "POZYTYWNY",
  "description": "Wykryto zwiększoną aktywność - powrót z pożytku",
  "details": {
    "bee_activity_index": 78.5,
    "swarm_probability": 12.3,
    "stress_indicator": 23.1,
    "dominant_frequency": 287.5,
    "power_bee_band": 567.8
  },
  "recent_events": [
    {
      "type": "NORMAL_ACTIVITY",
      "timestamp": "2024-01-15T10:25:00Z",
      "severity": "LOW",
      "impact": "POZYTYWNY",
      "description": "Normalna aktywność pszczół"
    },
    {
      "type": "INCREASED_ACTIVITY",
      "timestamp": "2024-01-15T10:28:00Z",
      "severity": "LOW",
      "impact": "POZYTYWNY",
      "description": "Zwiększona aktywność - dobry pożytek"
    }
  ]
}
```

```http
GET /audio/spectrum?resolution=low
Authorization: Bearer {token}

Response:
{
  "timestamp": "2024-01-15T10:30:00Z",
  "fft_size": 256,
  "sample_rate": 8000,
  "frequency_resolution": 31.25,
  "spectrum": [
    {"bin": 0, "frequency": 0.0, "magnitude": 0.001},
    {"bin": 1, "frequency": 31.25, "magnitude": 0.023},
    {"bin": 2, "frequency": 62.5, "magnitude": 0.045},
    ...
    {"bin": 127, "frequency": 3968.75, "magnitude": 0.012}
  ],
  "peaks": [
    {"frequency": 287.5, "magnitude": 0.089, "bin": 9},
    {"frequency": 456.25, "magnitude": 0.067, "bin": 14},
    {"frequency": 623.75, "magnitude": 0.054, "bin": 20}
  ]
}
```

```http
GET /audio/history?minutes=5
Authorization: Bearer {token}

Response:
{
  "timestamp": "2024-01-15T10:30:00Z",
  "interval_seconds": 1,
  "data_points": 300,
  "history": [
    {
      "timestamp": "2024-01-15T10:29:59Z",
      "rms_amplitude": 0.0234,
      "dominant_frequency": 287.5,
      "bee_activity_index": 78.5,
      "swarm_probability": 12.3,
      "event_type": "INCREASED_ACTIVITY"
    },
    ...
  ],
  "trends": {
    "activity_trend": "INCREASING",
    "swarm_risk_trend": "STABLE",
    "health_trend": "IMPROVING"
  }
}
```

#### Radar mmWave Endpoints (LD2410B)

```http
GET /radar/status
Authorization: Bearer {token}

Response:
{
  "timestamp": "2024-01-15T10:30:00Z",
  "firmware_version": "2.5.0",
  "uptime_seconds": 86400,
  "radar_connected": true,
  "buffer_size": 120,
  "samples_collected": 4521
}
```

```http
GET /radar/params
Authorization: Bearer {token}

Response:
{
  "timestamp": "2024-01-15T10:30:00Z",
  "distance_stats": {
    "mean_cm": 45.3,
    "median_cm": 44.8,
    "std_dev_cm": 12.7,
    "min_cm": 15.2,
    "max_cm": 98.4,
    "percentile_10_cm": 28.5,
    "percentile_90_cm": 67.2
  },
  "energy_analysis": {
    "total_energy": 1247.5,
    "energy_density": 10.4,
    "coefficient_of_variation": 0.34
  },
  "motion_dynamics": {
    "estimated_velocity_cm_s": 23.5,
    "acceleration_cm_s2": 4.2,
    "swarm_liveness_index": 7.8
  },
  "temporal_trends": {
    "trend_slope": 1.23,
    "linear_regression_r2": 0.87,
    "predicted_activity_5min": 8.2
  },
  "anomaly_detection": {
    "zscore_max": 1.8,
    "outliers_count": 2,
    "anomaly_score": 0.15
  },
  "quality_indices": {
    "hive_health_index": 8.5,
    "forage_status": "POZYTYWNY",
    "activity_level": "HIGH"
  }
}
```

```http
GET /radar/anomalies
Authorization: Bearer {token}

Response:
{
  "timestamp": "2024-01-15T10:30:00Z",
  "status": "POZYTYWNY",
  "event_type": "INTENSYWNY_OBLOT",
  "confidence": 0.92,
  "hive_health_index": 8.5,
  "anomaly_score": 0.1,
  "details": {
    "trend_slope": 1.2,
    "energy_change_percent": 15.4,
    "target_count_avg": 45,
    "zscore_current": 1.8
  },
  "recent_events": [
    {
      "type": "NAGLY_WZROST_RUCHU",
      "timestamp": "2024-01-15T10:25:00Z",
      "severity": "LOW",
      "description": "Wykryto nagły wzrost aktywności - powrót z pożytku"
    }
  ]
}
```

```http
GET /radar/raw?seconds=5
Authorization: Bearer {token}

Response:
{
  "timestamp": "2024-01-15T10:30:00Z",
  "samples": [
    {
      "timestamp": "2024-01-15T10:29:55Z",
      "distance_cm": 45.2,
      "energy": 127,
      "speed_cm_s": 22.5,
      "targets_count": 3
    },
    ...
  ]
}
```

#### Authentication
```http
POST /api/auth/login
Content-Type: application/json

{
  "username": "admin",
  "password": "secure_password"
}

Response:
{
  "token": "eyJhbGciOiJIUzI1NiIs...",
  "expiresIn": 3600,
  "refreshToken": "dGhpcyBpcyBhIHJlZnJlc2g..."
}
```

#### Hives Management
```http
GET /api/hives
Authorization: Bearer {token}

Response:
{
  "hives": [
    {
      "id": 1,
      "name": "Ul #1",
      "location": "Pasieka A",
      "status": "Active",
      "healthScore": 87.5,
      "lastReading": {
        "timestamp": "2024-01-15T10:30:00Z",
        "weight": 45.6,
        "temperature": 34.2,
        "humidity": 58.0
      }
    }
  ]
}

POST /api/hives
{
  "name": "Ul #15",
  "location": "Pasieka B",
  "initialWeight": 15.0
}

PUT /api/hives/{id}
DELETE /api/hives/{id}
```

#### Sensor Data
```http
GET /api/hives/{hiveId}/sensor-data?from=2024-01-01&to=2024-01-15&type=weight,temp

Response:
{
  "readings": [
    {
      "timestamp": "2024-01-15T10:00:00Z",
      "weight": 45.6,
      "temperature": 34.2,
      "humidity": 58.0,
      "audioLevel": 42.5
    }
  ]
}
```

#### Actuators Control
```http
POST /api/hives/{hiveId}/actuators/heater
{
  "action": "start",
  "targetTemperature": 40.0,
  "duration": 1440 // minutes
}

POST /api/hives/{hiveId}/actuators/fan
{
  "action": "set_speed",
  "speedPercent": 75
}

POST /api/hives/{hiveId}/actuators/dispenser
{
  "substance": "formic_acid",
  "volumeMl": 50.0,
  "schedule": "immediate"
}
```

#### Alerts
```http
GET /api/alerts?status=active&severity=critical

POST /api/alerts/{id}/acknowledge
POST /api/alerts/{id}/resolve
```

### MQTT Topics

```
apiaryguard/{hive_id}/telemetry       # QoS 1, Retain
  Payload: JSON with all sensor data

apiaryguard/{hive_id}/alerts          # QoS 2
  Payload: Alert object

apiaryguard/{hive_id}/commands/heater # QoS 1
  Payload: {"action": "start|stop", "target": 40.0}

apiaryguard/{hive_id}/commands/fan    # QoS 1
  Payload: {"speed": 0-100}

apiaryguard/{hive_id}/status          # QoS 1, Retain, LWT
  Payload: {"online": true, "lastSeen": "..."}

apiaryguard/broadcast/firmware_update # QoS 2
  Payload: {"version": "2.1.0", "url": "..."}
```

### Webhooks

**Konfigurowalne Events:**
- `hive.alert.created`
- `hive.swarm.detected`
- `hive.treatment.completed`
- `device.offline`
- `device.online`
- `daily.report.ready`

**Przykład Webhook Payload:**
```json
{
  "event": "hive.swarm.detected",
  "timestamp": "2024-01-15T14:30:00Z",
  "hive": {
    "id": 5,
    "name": "Ul #5",
    "location": "Pasieka A"
  },
  "data": {
    "probability": 0.87,
    "weightDrop": 2.3,
    "audioEvidence": "https://apiaryguard.local/recordings/swarm_20240115.wav",
    "recommendedAction": "Inspect within 24 hours"
  },
  "severity": "high"
}
```

---

