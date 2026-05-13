package com.apiaryguard.app;

/**
 * Model danych dla ula
 */
public class Apiary {
    private String id;
    private String name;
    private double temperature;
    private double humidity;
    private double weight;
    private String status;
    private String lastUpdate;
    
    public Apiary() {
        // Konstruktor domyślny wymagany przez Retrofit/GSON
    }
    
    public Apiary(String id, String name, double temperature, double humidity, 
                  double weight, String status, String lastUpdate) {
        this.id = id;
        this.name = name;
        this.temperature = temperature;
        this.humidity = humidity;
        this.weight = weight;
        this.status = status;
        this.lastUpdate = lastUpdate;
    }
    
    // Gettery i Settery
    public String getId() {
        return id;
    }
    
    public void setId(String id) {
        this.id = id;
    }
    
    public String getName() {
        return name;
    }
    
    public void setName(String name) {
        this.name = name;
    }
    
    public double getTemperature() {
        return temperature;
    }
    
    public void setTemperature(double temperature) {
        this.temperature = temperature;
    }
    
    public double getHumidity() {
        return humidity;
    }
    
    public void setHumidity(double humidity) {
        this.humidity = humidity;
    }
    
    public double getWeight() {
        return weight;
    }
    
    public void setWeight(double weight) {
        this.weight = weight;
    }
    
    public String getStatus() {
        return status;
    }
    
    public void setStatus(String status) {
        this.status = status;
    }
    
    public String getLastUpdate() {
        return lastUpdate;
    }
    
    public void setLastUpdate(String lastUpdate) {
        this.lastUpdate = lastUpdate;
    }
}
