package com.apiguard.apiary.ui

import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.ViewModelProvider
import androidx.recyclerview.widget.LinearLayoutManager
import com.apiguard.apiary.R
import com.apiguard.apiary.databinding.ActivityMainBinding
import com.apiguard.apiary.model.ApiaryData

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var viewModel: MainViewModel
    private lateinit var adapter: ApiaryAdapter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setupViewModel()
        setupRecyclerView()
        setupClickListeners()
        
        // Sprawdź czy mamy zapisany adres IP
        viewModel.checkSavedConnection()
    }

    private fun setupViewModel() {
        viewModel = ViewModelProvider(this, 
            MainViewModelFactory(applicationContext)
        )[MainViewModel::class.java]

        viewModel.connectionState.observe(this) { isConnected ->
            if (!isConnected) {
                showIpInputDialog()
            }
        }

        viewModel.isLoading.observe(this) { isLoading ->
            binding.swipeRefresh.isRefreshing = isLoading
        }

        viewModel.errorMessage.observe(this) { error ->
            error?.let {
                Toast.makeText(this, it, Toast.LENGTH_LONG).show()
                viewModel.clearError()
            }
        }

        viewModel.apiaryData.observe(this) { data ->
            adapter.submitList(data)
            binding.emptyState.visibility = if (data.isEmpty()) View.VISIBLE else View.GONE
            binding.recyclerView.visibility = if (data.isEmpty()) View.GONE else View.VISIBLE
        }
    }

    private fun setupRecyclerView() {
        adapter = ApiaryAdapter { apiaryData ->
            showApiaryDetails(apiaryData)
        }
        
        binding.recyclerView.apply {
            layoutManager = LinearLayoutManager(this@MainActivity)
            adapter = this@MainActivity.adapter
        }
    }

    private fun setupClickListeners() {
        binding.swipeRefresh.setOnRefreshListener {
            viewModel.loadData()
        }
        
        binding.btnChangeIp.setOnClickListener {
            showIpInputDialog()
        }
    }

    private fun showIpInputDialog() {
        val savedIp = viewModel.getSavedIpAddress() ?: ""
        val savedPort = viewModel.getSavedPort().toString()
        
        val view = layoutInflater.inflate(R.layout.dialog_ip_input, null)
        val ipEditText = view.findViewById<android.widget.EditText>(R.id.edit_ip_address)
        val portEditText = view.findViewById<android.widget.EditText>(R.id.edit_port)
        
        ipEditText.setText(savedIp)
        portEditText.setText(savedPort)
        
        AlertDialog.Builder(this)
            .setTitle("Konfiguracja połączenia")
            .setMessage("Podaj statyczny adres IP Raspberry Pi:")
            .setView(view)
            .setPositiveButton("Połącz") { _, _ ->
                val ipAddress = ipEditText.text.toString().trim()
                val port = portEditText.text.toString().toIntOrNull() ?: 5000
                
                if (ipAddress.isNotEmpty()) {
                    viewModel.verifyAndSaveConnection(ipAddress, port)
                } else {
                    Toast.makeText(this, "Podaj poprawny adres IP", Toast.LENGTH_SHORT).show()
                }
            }
            .setNegativeButton("Anuluj", null)
            .show()
    }

    private fun showApiaryDetails(apiaryData: ApiaryData) {
        AlertDialog.Builder(this)
            .setTitle(apiaryData.name)
            .setMessage(
                "Temperatura: ${apiaryData.temperature}°C\n" +
                "Wilgotność: ${apiaryData.humidity}%\n" +
                "Waga: ${apiaryData.weight}kg\n" +
                "Bateria: ${apiaryData.battery}%"
            )
            .setPositiveButton("OK", null)
            .show()
    }
}
