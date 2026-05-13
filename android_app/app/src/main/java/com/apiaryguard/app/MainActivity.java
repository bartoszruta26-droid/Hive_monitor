package com.apiaryguard.app;

import android.os.Bundle;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.ViewModelProvider;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.swiperefreshlayout.widget.SwipeRefreshLayout;
import com.google.android.material.floatingactionbutton.FloatingActionButton;

/**
 * Główna aktywność aplikacji ApiaryGuard do monitorowania uli
 */
public class MainActivity extends AppCompatActivity {
    
    private RecyclerView recyclerView;
    private ApiaryAdapter adapter;
    private SwipeRefreshLayout swipeRefreshLayout;
    private TextView statusTextView;
    private FloatingActionButton refreshButton;
    private MainViewModel viewModel;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        // Inicjalizacja widoków
        initViews();
        
        // Konfiguracja ViewModel
        viewModel = new ViewModelProvider(this).get(MainViewModel.class);
        
        // Konfiguracja RecyclerView
        setupRecyclerView();
        
        // Konfiguracja SwipeRefreshLayout
        setupSwipeRefresh();
        
        // Konfiguracja przycisku odświeżania
        setupRefreshButton();
        
        // Obserwowanie danych z ViewModel
        observeData();
        
        // Ładowanie danych
        loadApiaries();
    }
    
    private void initViews() {
        recyclerView = findViewById(R.id.recyclerView);
        swipeRefreshLayout = findViewById(R.id.swipeRefreshLayout);
        statusTextView = findViewById(R.id.statusTextView);
        refreshButton = findViewById(R.id.refreshButton);
    }
    
    private void setupRecyclerView() {
        adapter = new ApiaryAdapter(this);
        recyclerView.setLayoutManager(new LinearLayoutManager(this));
        recyclerView.setAdapter(adapter);
    }
    
    private void setupSwipeRefresh() {
        swipeRefreshLayout.setOnRefreshListener(() -> {
            loadApiaries();
        });
        swipeRefreshLayout.setColorSchemeResources(
            R.color.primary,
            R.color.primary_dark,
            R.color.accent
        );
    }
    
    private void setupRefreshButton() {
        refreshButton.setOnClickListener(v -> loadApiaries());
    }
    
    private void observeData() {
        viewModel.getApiaries().observe(this, apiaries -> {
            if (apiaries != null && !apiaries.isEmpty()) {
                adapter.setApiaries(apiaries);
                statusTextView.setVisibility(View.GONE);
                recyclerView.setVisibility(View.VISIBLE);
            } else {
                statusTextView.setText("Brak danych o ulach");
                statusTextView.setVisibility(View.VISIBLE);
                recyclerView.setVisibility(View.GONE);
            }
        });
        
        viewModel.getLoadingState().observe(this, isLoading -> {
            swipeRefreshLayout.setRefreshing(isLoading);
        });
        
        viewModel.getError().observe(this, error -> {
            if (error != null) {
                Toast.makeText(this, error, Toast.LENGTH_LONG).show();
                viewModel.clearError();
            }
        });
    }
    
    private void loadApiaries() {
        viewModel.loadApiaries();
    }
}
