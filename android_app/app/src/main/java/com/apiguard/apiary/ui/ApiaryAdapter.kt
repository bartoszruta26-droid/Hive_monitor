package com.apiguard.apiary.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.cardview.widget.CardView
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.RecyclerView
import com.apiguard.apiary.R
import com.apiguard.apiary.model.ApiaryData
import java.text.SimpleDateFormat
import java.util.*

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
            cardTemperature.text = "${apiaryData.temperature}°C"
            cardHumidity.text = "${apiaryData.humidity}%"
            cardWeight.text = "${apiaryData.weight} kg"
            cardBattery.text = "${apiaryData.battery}%"
            
            val dateFormat = SimpleDateFormat("dd.MM.yyyy HH:mm", Locale.getDefault())
            cardTimestamp.text = "Ostatnia aktualizacja: ${dateFormat.format(Date(apiaryData.timestamp))}"
            
            // Status kolorystyczny
            val (statusText, statusColor) = determineStatus(apiaryData)
            cardStatus.text = statusText
            cardStatus.setTextColor(statusColor)
            
            cardView.setOnClickListener {
                onItemClick(apiaryData)
            }
        }

        private fun determineStatus(data: ApiaryData): Pair<String, Int> {
            return when {
                data.temperature < 10 || data.temperature > 40 -> 
                    "ALERT" to itemView.context.getColor(R.color.status_alert)
                data.humidity < 30 || data.humidity > 80 -> 
                    "UWAGA" to itemView.context.getColor(R.color.status_warning)
                data.battery < 20 -> 
                    "UWAGA" to itemView.context.getColor(R.color.status_warning)
                else -> 
                    "OK" to itemView.context.getColor(R.color.status_ok)
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
