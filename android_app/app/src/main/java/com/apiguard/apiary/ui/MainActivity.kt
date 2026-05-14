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
        
        ipEditText.setText(savedIp)
        portEditText.setText(savedPort)
        
        AlertDialog.Builder(this)
            .setTitle("Konfiguracja połączenia")
            .setMessage("Podaj statyczny adres IP Raspberry Pi:")
            .setView(view)
            .setPositiveButton("Połącz") { _, _ ->
                val ipAddress = ipEditText.text.toString().trim()
                val portText = portEditText.text.toString()
                
                // Walidacja adresu IP
                if (ipAddress.isEmpty()) {
                    Toast.makeText(this, "Podaj poprawny adres IP", Toast.LENGTH_SHORT).show()
                    isIpDialogShowing = false
                    return@setPositiveButton
                }
                
                // Walidacja formatu IP
                if (!isValidIpAddress(ipAddress)) {
                    Toast.makeText(this, "Nieprawidłowy format adresu IP", Toast.LENGTH_SHORT).show()
                    isIpDialogShowing = false
                    return@setPositiveButton
                }
                
                // Walidacja portu
                val port = portText.toIntOrNull()
                if (port == null || port < 1 || port > 65535) {
                    Toast.makeText(this, "Port musi być w zakresie 1-65535", Toast.LENGTH_SHORT).show()
                    isIpDialogShowing = false
                    return@setPositiveButton
                }
                
                viewModel.verifyAndSaveConnection(ipAddress, port)
            }
            .setNegativeButton("Anuluj") { _, _ ->
                isIpDialogShowing = false
            }
            .setOnDismissListener {
                isIpDialogShowing = false
            }
            .show()
        
        // Oznacz że dialog jest wyświetlany
        isIpDialogShowing = true
    }

    /**
     * Waliduje format adresu IPv4
     */
    private fun isValidIpAddress(ip: String): Boolean {
        val ipPattern = Regex(
            "^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}" +
            "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$"
        )
        return ipPattern.matches(ip)
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
