package com.apiaryguard.app;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import java.util.ArrayList;
import java.util.List;

/**
 * Adapter do wyświetlania listy uli w RecyclerView
 */
public class ApiaryAdapter extends RecyclerView.Adapter<ApiaryAdapter.ApiaryViewHolder> {
    
    private Context context;
    private List<Apiary> apiaries = new ArrayList<>();
    
    public ApiaryAdapter(Context context) {
        this.context = context;
    }
    
    @NonNull
    @Override
    public ApiaryViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(context).inflate(R.layout.item_apiary, parent, false);
        return new ApiaryViewHolder(view);
    }
    
    @Override
    public void onBindViewHolder(@NonNull ApiaryViewHolder holder, int position) {
        Apiary apiary = apiaries.get(position);
        holder.bind(apiary);
    }
    
    @Override
    public int getItemCount() {
        return apiaries.size();
    }
    
    public void setApiaries(List<Apiary> apiaries) {
        this.apiaries = apiaries != null ? apiaries : new ArrayList<>();
        notifyDataSetChanged();
    }
    
    static class ApiaryViewHolder extends RecyclerView.ViewHolder {
        private TextView nameTextView;
        private TextView temperatureTextView;
        private TextView humidityTextView;
        private TextView weightTextView;
        private TextView statusTextView;
        private TextView lastUpdateTextView;
        
        public ApiaryViewHolder(@NonNull View itemView) {
            super(itemView);
            nameTextView = itemView.findViewById(R.id.nameTextView);
            temperatureTextView = itemView.findViewById(R.id.temperatureTextView);
            humidityTextView = itemView.findViewById(R.id.humidityTextView);
            weightTextView = itemView.findViewById(R.id.weightTextView);
            statusTextView = itemView.findViewById(R.id.statusTextView);
            lastUpdateTextView = itemView.findViewById(R.id.lastUpdateTextView);
        }
        
        public void bind(Apiary apiary) {
            nameTextView.setText(apiary.getName());
            temperatureTextView.setText(String.format("Temp: %.1f°C", apiary.getTemperature()));
            humidityTextView.setText(String.format("Wilgotność: %.1f%%", apiary.getHumidity()));
            weightTextView.setText(String.format("Waga: %.2f kg", apiary.getWeight()));
            
            // Status kolorystyczny
            String status = apiary.getStatus();
            statusTextView.setText(status);
            if ("OK".equals(status)) {
                statusTextView.setTextColor(context.getResources().getColor(android.R.color.holo_green_dark));
            } else if ("UWAGA".equals(status)) {
                statusTextView.setTextColor(context.getResources().getColor(android.R.color.holo_orange_dark));
            } else {
                statusTextView.setTextColor(context.getResources().getColor(android.R.color.holo_red_dark));
            }
            
            lastUpdateTextView.setText("Ostatnia aktualizacja: " + apiary.getLastUpdate());
        }
    }
}
