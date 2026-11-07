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
import java.util.Locale

/**
 * Adapter dla RecyclerView wyświetlający listę apiary (pasiek).
 * Odpowiada za renderowanie pojedynczych elementów listy oraz obsługę kliknięć.
 * 
 * @param onItemClick callback wywoływany po kliknięciu w element listy
 */
class ApiaryAdapter(
    private val onItemClick: (ApiaryData) -> Unit
) : RecyclerView.Adapter<ApiaryAdapter.ApiaryViewHolder>() {

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
            cardTemperature.text = if (apiaryData.available) String.format(Locale.getDefault(), "%.1f°C", apiaryData.temperature) else "-- °C"
            cardHumidity.text = if (apiaryData.available) String.format(Locale.getDefault(), "%.1f%%", apiaryData.humidity) else "-- %"
            cardWeight.text = if (apiaryData.available) String.format(Locale.getDefault(), "%.2f kg", apiaryData.weight) else "-- kg"
            cardBattery.text = if (apiaryData.available) "${apiaryData.battery}%" else "--%"
            
            // Bezpieczne formatowanie timestamp z walidacją
            cardTimestamp.text = "Ostatnia aktualizacja: ${ApiaryUiFormatter.formatTimestamp(apiaryData.timestamp)}"
            
            // Status kolorystyczny
            val statusText = ApiaryUiFormatter.determineStatus(apiaryData)
            val statusColor = when (statusText) {
                "OFFLINE", "ALERT" -> R.color.status_alert
                "BRAK DANYCH", "UWAGA" -> R.color.status_warning
                else -> R.color.status_ok
            }
            cardStatus.text = statusText
            cardStatus.setTextColor(ContextCompat.getColor(itemView.context, statusColor))
            
            cardView.setOnClickListener {
                onItemClick(apiaryData)
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
        // Upewnij się że operacja jest wykonywana na głównym wątku
        if (android.os.Looper.myLooper() != android.os.Looper.getMainLooper()) {
            throw IllegalStateException("submitList must be called on the main thread")
        }
        
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
