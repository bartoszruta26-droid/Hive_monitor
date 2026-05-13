package com.apiguard.apiary.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.cardview.widget.CardView
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.RecyclerView
import com.apiguard.apiary.R
import com.apiguard.apiary.model.ApiaryData
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.concurrent.TimeUnit

class ApiaryAdapter(
    private val onItemClick: (ApiaryData) -> Unit
) : RecyclerView.Adapter<ApiaryAdapter.ApiaryViewHolder>() {

    companion object {
        private val dateFormat = SimpleDateFormat("dd.MM.yyyy HH:mm", Locale.getDefault())
        
        // Stałe dla progów statusu - łatwe do konfiguracji
        private const val TEMP_MIN_THRESHOLD = 10
        private const val TEMP_MAX_THRESHOLD = 40
        private const val HUMIDITY_MIN_THRESHOLD = 30
        private const val HUMIDITY_MAX_THRESHOLD = 80
        private const val BATTERY_LOW_THRESHOLD = 20
        
        // Cache na ostatnio sformatowaną datę aby uniknąć powtarzających się operacji
        private var lastTimestamp: Long = -1
        private var lastFormattedDate: String = ""
    }

    private var apiaryList = emptyList<ApiaryData>()

    inner class ApiaryViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        private val cardName: TextView = itemView.findViewById(R.id.card_name)
        private val cardTemperature: TextView = itemView.findViewById(R.id.card_temperature)
        private val cardHumidity: TextView = itemView.findViewById(R.id.card_humidity)
        private val cardWeight: TextView = itemView.findViewById(R.id.card_weight)
        private val cardBattery: TextView = itemView.findViewById(R.id.card_battery)
        private val cardTimestamp: TextView = itemView.findViewById(R.id.card_timestamp)
        private val cardStatus: TextView = itemView.findViewById(R.id.card_status)
        private val cardView: CardView = itemView.findViewById(R.id.card_view)

        fun bind(apiaryData: ApiaryData) {
            cardName.text = apiaryData.name
            cardTemperature.text = "${apiaryData.temperature}°C"
            cardHumidity.text = "${apiaryData.humidity}%"
            cardWeight.text = "${apiaryData.weight} kg"
            cardBattery.text = "${apiaryData.battery}%"
            
            // Bezpieczne formatowanie timestamp z walidacją
            cardTimestamp.text = "Ostatnia aktualizacja: ${formatTimestamp(apiaryData.timestamp)}"
            
            // Status kolorystyczny
            val (statusText, statusColor) = determineStatus(apiaryData)
            cardStatus.text = statusText
            cardStatus.setTextColor(statusColor)
            
            cardView.setOnClickListener {
                onItemClick(apiaryData)
            }
        }

        private fun formatTimestamp(timestamp: Long): String {
            // Walidacja timestamp
            if (timestamp <= 0) {
                return "brak danych"
            }
            
            // Optymalizacja: cache dla tego samego timestamp
            if (timestamp == lastTimestamp) {
                return lastFormattedDate
            }
            
            try {
                lastTimestamp = timestamp
                lastFormattedDate = dateFormat.format(Date(timestamp))
                return lastFormattedDate
            } catch (e: Exception) {
                // W przypadku błędu formatowania zwracamy surowy timestamp
                return "błąd daty"
            }
        }

        private fun determineStatus(data: ApiaryData): Pair<String, Int> {
            return when {
                data.temperature < TEMP_MIN_THRESHOLD || data.temperature > TEMP_MAX_THRESHOLD -> 
                    "ALERT" to ContextCompat.getColor(itemView.context, R.color.status_alert)
                data.humidity < HUMIDITY_MIN_THRESHOLD || data.humidity > HUMIDITY_MAX_THRESHOLD -> 
                    "UWAGA" to ContextCompat.getColor(itemView.context, R.color.status_warning)
                data.battery < BATTERY_LOW_THRESHOLD -> 
                    "UWAGA" to ContextCompat.getColor(itemView.context, R.color.status_warning)
                else -> 
                    "OK" to ContextCompat.getColor(itemView.context, R.color.status_ok)
            }
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ApiaryViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_apiary, parent, false)
        return ApiaryViewHolder(view)
    }

    override fun onBindViewHolder(holder: ApiaryViewHolder, position: Int) {
        holder.bind(apiaryList[position])
    }

    override fun getItemCount() = apiaryList.size

    fun submitList(newList: List<ApiaryData>) {
        val diffCallback = ApiaryDiffCallback(apiaryList, newList)
        val diffResult = DiffUtil.calculateDiff(diffCallback)
        apiaryList = newList
        diffResult.dispatchUpdatesTo(this)
    }

    class ApiaryDiffCallback(
        private val oldList: List<ApiaryData>,
        private val newList: List<ApiaryData>
    ) : DiffUtil.Callback() {
        override fun getOldListSize() = oldList.size
        override fun getNewListSize() = newList.size
        override fun areItemsTheSame(oldItemPosition: Int, newItemPosition: Int) =
            oldList[oldItemPosition].id == newList[newItemPosition].id
        override fun areContentsTheSame(oldItemPosition: Int, newItemPosition: Int) =
            oldList[oldItemPosition] == newList[newItemPosition]
    }
}
