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
import com.apiguard.apiary.util.isValidIpAddress
import com.apiguard.apiary.util.isValidPort

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var viewModel: MainViewModel
    private lateinit var adapter: ApiaryAdapter
    
    // Klucze do zapisywania stanu
    companion object {
        private const val KEY_SCROLL_POSITION = "scroll_position"
        private const val KEY_DIALOG_SHOWING = "dialog_showing"
    }
    
    // Zmienna do śledzenia czy dialog jest wyświetlany
    private var isIpDialogShowing = false
    private var scrollPosition = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setupViewModel()
        setupRecyclerView()
        setupClickListeners()
        
        // Przywróć stan z savedInstanceState jeśli istnieje
        savedInstanceState?.let {
            scrollPosition = it.getInt(KEY_SCROLL_POSITION, 0)
            isIpDialogShowing = it.getBoolean(KEY_DIALOG_SHOWING, false)
            
            // Przywróć pozycję przewijania
            if (scrollPosition > 0) {
                binding.recyclerView.scrollToPosition(scrollPosition)
            }
            
            // Reset flagi dialogu - Observer i tak wywoła showIpInputDialog() jeśli potrzeba
        }
        
        // Sprawdź czy mamy zapisany adres IP przy każdym uruchomieniu (również po odtworzeniu stanu)
        // Nowy MainViewModel zaczyna z nieustawionym stanem połączenia, więc musimy sprawdzić zapisane dane
        viewModel.checkSavedConnection()
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        
        // Zapisz pozycję przewijania listy
        val layoutManager = binding.recyclerView.layoutManager as? LinearLayoutManager
        scrollPosition = layoutManager?.findFirstVisibleItemPosition() ?: 0
        outState.putInt(KEY_SCROLL_POSITION, scrollPosition)
        
        // Zapisz stan dialogu
        outState.putBoolean(KEY_DIALOG_SHOWING, isIpDialogShowing)
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
        // Jeśli dialog już jest wyświetlany, nie pokazuj ponownie (zapobiega wyciekowi przy rotacji)
        if (isIpDialogShowing) return
        
        val savedIp = viewModel.getSavedIpAddress() ?: ""
        val savedPort = viewModel.getSavedPort().toString()
        
        val view = layoutInflater.inflate(R.layout.dialog_ip_input, null)
        val ipEditText = view.findViewById<android.widget.EditText>(R.id.edit_ip_address)
        val portEditText = view.findViewById<android.widget.EditText>(R.id.edit_port)
        val ipTextInputLayout = view.findViewById<com.google.android.material.textfield.TextInputLayout>(R.id.text_input_layout_ip)
        val portTextInputLayout = view.findViewById<com.google.android.material.textfield.TextInputLayout>(R.id.text_input_layout_port)
        
        ipEditText.setText(savedIp)
        portEditText.setText(savedPort)
        
        var dialog: AlertDialog? = null
        
        dialog = AlertDialog.Builder(this)
            .setTitle(R.string.dialog_connection_title)
            .setMessage(R.string.dialog_connection_message)
            .setView(view)
            .setPositiveButton(R.string.dialog_connect_button) { _, _ ->
                val ipAddress = ipEditText.text.toString().trim()
                val portText = portEditText.text.toString()
                
                // Resetowanie błędów
                ipTextInputLayout?.error = null
                portTextInputLayout?.error = null
                
                // Walidacja adresu IP - użyj extension function z AppConstants
                if (ipAddress.isEmpty()) {
                    ipTextInputLayout?.error = getString(R.string.error_ip_empty)
                    // Nie zamykaj dialogu - pozwól użytkownikowi poprawić dane
                    return@setPositiveButton
                }
                
                // Walidacja formatu IP - użyj extension function z AppConstants
                if (!ipAddress.isValidIpAddress()) {
                    ipTextInputLayout?.error = getString(R.string.error_ip_invalid_format)
                    // Nie zamykaj dialogu - pozwól użytkownikowi poprawić dane
                    return@setPositiveButton
                }
                
                // Walidacja portu - użyj stałych z AppConstants
                val port = portText.toIntOrNull()
                if (port == null || !port.isValidPort()) {
                    portTextInputLayout?.error = getString(R.string.error_port_invalid_range)
                    // Nie zamykaj dialogu - pozwól użytkownikowi poprawić dane
                    return@setPositiveButton
                }
                
                // Tylko przy poprawnych danych zamknij dialog i zapisz połączenie
                isIpDialogShowing = false
                viewModel.verifyAndSaveConnection(ipAddress, port)
            }
            .setNegativeButton(R.string.dialog_cancel_button) { _, _ ->
                isIpDialogShowing = false
            }
            .setOnDismissListener {
                isIpDialogShowing = false
            }
            .create()
        
        dialog.show()
        
        // Oznacz że dialog jest wyświetlany
        isIpDialogShowing = true
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
