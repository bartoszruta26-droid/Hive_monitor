package com.apiguard.apiary.ui

import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.repeatOnLifecycle
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import androidx.recyclerview.widget.LinearLayoutManager
import com.apiguard.apiary.R
import com.apiguard.apiary.databinding.ActivityMainBinding
import com.apiguard.apiary.data.model.ApiaryReading
import java.util.Calendar
import java.util.TimeZone
import com.apiguard.apiary.model.ApiaryData
import com.apiguard.apiary.util.isValidApiHost
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
        startTelemetryRefresh()
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

    private fun startTelemetryRefresh() {
        lifecycleScope.launch {
            repeatOnLifecycle(Lifecycle.State.STARTED) {
                while (true) {
                    delay(10_000L)
                    viewModel.refreshConnectionAndData()
                }
            }
        }
    }

    private fun setupViewModel() {
        viewModel = ViewModelProvider(this, 
            MainViewModelFactory(application)
        )[MainViewModel::class.java]

        viewModel.connectionState.observe(this) { isConnected ->
            val hasSavedHost = viewModel.getSavedIpAddress() != null
            renderConnectionBanner(isConnected == true, hasSavedHost)
            if (!isConnected && !hasSavedHost) {
                showIpInputDialog()
            }
        }

        viewModel.archivedReadingsInfo.observe(this) {
            renderConnectionBanner(viewModel.connectionState.value == true, viewModel.getSavedIpAddress() != null)
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

        viewModel.history.observe(this) { history ->
            if (history != null) {
                AlertDialog.Builder(this)
                    .setTitle(getString(R.string.history_comparison_title, history.hiveId))
                    .setMessage(ApiaryUiFormatter.formatHistoryYearComparison(history))
                    .setPositiveButton(android.R.string.ok, null)
                    .show()
            }
        }
    }

    private fun renderConnectionBanner(isConnected: Boolean, hasSavedHost: Boolean) {
        val info = viewModel.archivedReadingsInfo.value
        binding.connectionBanner.visibility = if (!isConnected && hasSavedHost) View.VISIBLE else View.GONE
        binding.connectionBanner.text = if (!isConnected && hasSavedHost && info != null) {
            getString(R.string.connection_offline_with_archive, info.count, ApiaryUiFormatter.formatTimestamp(info.latestTimestamp))
        } else {
            getString(R.string.connection_offline_banner)
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
        
        binding.connectionBanner.setOnClickListener {
            if (viewModel.connectionState.value != true) showArchivedReadings()
        }

        binding.btnChangeIp.setOnClickListener {
            showIpInputDialog()
        }
    }

    private fun showIpInputDialog() {
        if (isIpDialogShowing) return

        val savedIp = viewModel.getSavedIpAddress() ?: ""
        val savedPort = viewModel.getSavedPort().toString()
        val dialogView = layoutInflater.inflate(R.layout.dialog_ip_input, null)
        val ipEditText = dialogView.findViewById<android.widget.EditText>(R.id.edit_ip_address)
        val portEditText = dialogView.findViewById<android.widget.EditText>(R.id.edit_port)
        val ipTextInputLayout = dialogView.findViewById<com.google.android.material.textfield.TextInputLayout>(R.id.text_input_layout_ip)
        val portTextInputLayout = dialogView.findViewById<com.google.android.material.textfield.TextInputLayout>(R.id.text_input_layout_port)

        ipEditText.setText(savedIp)
        portEditText.setText(savedPort)

        val dialog = AlertDialog.Builder(this)
            .setTitle(R.string.dialog_connection_title)
            .setMessage(R.string.dialog_connection_message)
            .setView(dialogView)
            // Listener ustawiamy po show(), aby błędne dane nie zamykały okna.
            .setPositiveButton(R.string.dialog_connect_button, null)
            .setNegativeButton(R.string.dialog_cancel_button) { _, _ ->
                isIpDialogShowing = false
            }
            .setOnDismissListener {
                isIpDialogShowing = false
            }
            .create()

        dialog.setOnShowListener {
            dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener {
                val ipAddress = ipEditText.text.toString().trim()
                val port = portEditText.text.toString().trim().toIntOrNull()

                ipTextInputLayout.error = null
                portTextInputLayout.error = null

                when {
                    ipAddress.isEmpty() -> ipTextInputLayout.error = getString(R.string.error_ip_empty)
                    !ipAddress.isValidApiHost() -> ipTextInputLayout.error = getString(R.string.error_ip_invalid_format)
                    port == null || !port.isValidPort() -> portTextInputLayout.error = getString(R.string.error_port_invalid_range)
                    else -> {
                        isIpDialogShowing = false
                        dialog.dismiss()
                        viewModel.verifyAndSaveConnection(ipAddress, port)
                    }
                }
            }
        }

        isIpDialogShowing = true
        dialog.show()
    }

    private fun showArchivedReadings() {
        val readings = viewModel.archivedReadings.value.orEmpty()
        if (readings.isEmpty()) {
            Toast.makeText(this, R.string.archive_empty, Toast.LENGTH_SHORT).show()
            return
        }
        val apiaryIds = ApiaryUiFormatter.archivedApiaryIds(readings)
        AlertDialog.Builder(this)
            .setTitle(R.string.archive_select_apiary)
            .setItems(apiaryIds.map { getString(R.string.archive_apiary_item, it) }.toTypedArray()) { _, position ->
                showArchivedTimeRange(readings, apiaryIds[position])
            }
            .setPositiveButton(android.R.string.ok, null)
            .show()
    }

    private fun showArchivedTimeRange(readings: List<ApiaryReading>, apiaryId: String) {
        val ranges = listOf(1, 3, 7)
        val labels = arrayOf(
            getString(R.string.archive_range_24h),
            getString(R.string.archive_range_3d),
            getString(R.string.archive_range_7d)
        )
        AlertDialog.Builder(this)
            .setTitle(getString(R.string.archive_range_title, apiaryId))
            .setItems(labels) { _, position ->
                val since = System.currentTimeMillis() - ranges[position] * 24L * 60L * 60L * 1000L
                val filtered = ApiaryUiFormatter.filterArchivedReadings(readings, apiaryId, since)
                showArchivedReadingList(filtered)
            }
            .setNegativeButton(android.R.string.cancel, null)
            .show()
    }

    private fun showArchivedReadingList(readings: List<ApiaryReading>) {
        if (readings.isEmpty()) {
            Toast.makeText(this, R.string.archive_range_empty, Toast.LENGTH_SHORT).show()
            return
        }
        val rows = readings.map { ApiaryUiFormatter.formatArchivedReading(it) }
        AlertDialog.Builder(this)
            .setTitle(R.string.archive_dialog_title)
            .setItems(rows.toTypedArray()) { _, position -> showArchivedReadingDetails(readings[position]) }
            .setPositiveButton(android.R.string.ok, null)
            .show()
    }

    private fun showArchivedReadingDetails(reading: ApiaryReading) {
        val details = getString(
            R.string.archive_reading_details,
            ApiaryUiFormatter.formatTimestamp(reading.timestamp),
            reading.temperature,
            reading.humidity,
            reading.weight,
            reading.batteryLevel
        )
        AlertDialog.Builder(this)
            .setTitle(R.string.archive_dialog_title)
            .setMessage(details)
            .setPositiveButton(android.R.string.ok, null)
            .show()
    }

    private fun showApiaryDetails(apiaryData: ApiaryData) {
        AlertDialog.Builder(this)
            .setTitle(apiaryData.name)
            .setMessage(
                "Status: ${if (apiaryData.online) "ONLINE" else "OFFLINE"}\n" +
                if (apiaryData.available) {
                    "Temperatura: ${apiaryData.temperature}°C\n" +
                    "Wilgotność: ${apiaryData.humidity}%\n" +
                    "Waga: ${apiaryData.weight} kg\n" +
                    "Bateria: ${apiaryData.battery}%"
                } else {
                    "Brak kompletnego zestawu aktualnych pomiarów."
                }
            )
            .setNeutralButton(R.string.history_compare_button) { _, _ ->
                showHistoryComparison(apiaryData.id)
            }
            .setPositiveButton("OK", null)
            .show()
    }

    private fun showHistoryComparison(apiaryId: String) {
        val aggregations = arrayOf("HOUR", "DAY", "WEEK", "MONTH", "QUARTER", "YEAR")
        val labels = arrayOf(
            getString(R.string.history_hourly),
            getString(R.string.history_daily),
            getString(R.string.history_weekly),
            getString(R.string.history_monthly),
            getString(R.string.history_quarterly),
            getString(R.string.history_yearly)
        )
        AlertDialog.Builder(this)
            .setTitle(R.string.history_select_aggregation)
            .setItems(labels) { _, aggregationIndex ->
                val yearOptions = arrayOf("2", "3", "5")
                AlertDialog.Builder(this)
                    .setTitle(R.string.history_select_years)
                    .setItems(yearOptions) { _, yearIndex ->
                        val end = System.currentTimeMillis() / 1000L
                        val calendar = Calendar.getInstance(TimeZone.getTimeZone("UTC"))
                        calendar.timeInMillis = end * 1000L
                        calendar.add(Calendar.YEAR, -yearOptions[yearIndex].toInt())
                        viewModel.loadHistory(apiaryId, aggregations[aggregationIndex], calendar.timeInMillis / 1000L, end)
                    }
                    .setNegativeButton(android.R.string.cancel, null)
                    .show()
            }
            .setNegativeButton(android.R.string.cancel, null)
            .show()
    }
}
