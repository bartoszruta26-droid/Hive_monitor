package com.apiaryguard.app;

import android.app.Application;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/**
 * ViewModel dla głównej aktywności
 * Zarządza danymi o ulach i ich stanem
 */
public class MainViewModel extends AndroidViewModel {
    
    private final MutableLiveData<List<Apiary>> apiaries = new MutableLiveData<>();
    private final MutableLiveData<Boolean> loadingState = new MutableLiveData<>();
    private final MutableLiveData<String> error = new MutableLiveData<>();
    
    public MainViewModel(Application application) {
        super(application);
    }
    
    public LiveData<List<Apiary>> getApiaries() {
        return apiaries;
    }
    
    public LiveData<Boolean> getLoadingState() {
        return loadingState;
    }
    
    public LiveData<String> getError() {
        return error;
    }
    
    public void clearError() {
        error.setValue(null);
    }
    
    /**
     * Ładuje dane o ulach
     * W wersji demonstracyjnej generuje losowe dane
     * W produkcji należy połączyć się z API przez Retrofit
     */
    public void loadApiaries() {
        loadingState.setValue(true);
        
        // Symulacja opóźnienia sieciowego
        new Thread(() -> {
            try {
                Thread.sleep(1000);
                
                // Generowanie przykładowych danych
                List<Apiary> mockApiaries = generateMockApiaries();
                
                apiaries.postValue(mockApiaries);
                loadingState.postValue(false);
                
            } catch (Exception e) {
                error.postValue("Błąd ładowania danych: " + e.getMessage());
                loadingState.postValue(false);
            }
        }).start();
    }
    
    /**
     * Generuje przykładowe dane dla demonstracji
     */
    private List<Apiary> generateMockApiaries() {
        List<Apiary> apiariesList = new ArrayList<>();
        Random random = new Random();
        
        // Ul 1
        Apiary apiary1 = new Apiary();
        apiary1.setId("1");
        apiary1.setName("Ul nr 1 - Pasieka Główna");
        apiary1.setTemperature(34.5 + random.nextDouble() * 2);
        apiary1.setHumidity(55 + random.nextDouble() * 10);
        apiary1.setWeight(45 + random.nextDouble() * 10);
        apiary1.setStatus("OK");
        apiary1.setLastUpdate(getCurrentTime());
        apiariesList.add(apiary1);
        
        // Ul 2
        Apiary apiary2 = new Apiary();
        apiary2.setId("2");
        apiary2.setName("Ul nr 2 - Pasieka Główna");
        apiary2.setTemperature(35.0 + random.nextDouble() * 2);
        apiary2.setHumidity(50 + random.nextDouble() * 10);
        apiary2.setWeight(50 + random.nextDouble() * 10);
        apiary2.setStatus("OK");
        apiary2.setLastUpdate(getCurrentTime());
        apiariesList.add(apiary2);
        
        // Ul 3
        Apiary apiary3 = new Apiary();
        apiary3.setId("3");
        apiary3.setName("Ul nr 3 - Pasieka Leśna");
        apiary3.setTemperature(36.5 + random.nextDouble() * 2);
        apiary3.setHumidity(60 + random.nextDouble() * 10);
        apiary3.setWeight(42 + random.nextDouble() * 10);
        apiary3.setStatus("UWAGA");
        apiary3.setLastUpdate(getCurrentTime());
        apiariesList.add(apiary3);
        
        return apiariesList;
    }
    
    private String getCurrentTime() {
        long currentTime = System.currentTimeMillis();
        return android.text.format.DateFormat.format("yyyy-MM-dd HH:mm:ss", currentTime).toString();
    }
}
