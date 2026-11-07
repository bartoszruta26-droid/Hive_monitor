package com.apiguard.apiary.ui

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.Path
import android.util.AttributeSet
import android.view.View
import com.apiguard.apiary.data.model.ApiaryMetricAggregate
import java.util.Calendar
import java.util.Locale
import java.util.TimeZone
import kotlin.math.max

/**
 * Wykres porównujący przebieg wybranego parametru telemetrycznego
 * w kolejnych latach. Oś X jest wyrównana według miesiąca i dnia.
 */
class TelemetryHistoryChartView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null
) : View(context, attrs) {
    private val axisPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = 0xff667085.toInt()
        strokeWidth = resources.displayMetrics.density
        style = Paint.Style.STROKE
    }
    private val gridPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = 0xffe4e7ec.toInt()
        strokeWidth = resources.displayMetrics.density
    }
    private val textPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = 0xff344054.toInt()
        textSize = 11f * resources.displayMetrics.scaledDensity
    }
    private val linePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 2.5f * resources.displayMetrics.density
        strokeCap = Paint.Cap.ROUND
        strokeJoin = Paint.Join.ROUND
    }
    private val palette = intArrayOf(
        0xff2563eb.toInt(), 0xffdc6b20.toInt(), 0xff15966b.toInt(),
        0xff9333ea.toInt(), 0xffdb2777.toInt(), 0xff64748b.toInt()
    )
    private var samples: List<ApiaryMetricAggregate> = emptyList()
    private var metricName: String = ""

    fun submit(metric: String, values: List<ApiaryMetricAggregate>) {
        metricName = metric
        samples = values.filter { it.name == metric && it.average.isFinite() }
        contentDescription = "Wykres archiwalny parametru $metric, porównanie lat"
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val density = resources.displayMetrics.density
        val left = 58f * density
        val right = width - 16f * density
        val top = 18f * density
        val bottom = height - 48f * density
        if (right <= left || bottom <= top) return
        val years = samples.groupBy { utcCalendar(it.timestampStart).get(Calendar.YEAR) }.toSortedMap()
        if (years.isEmpty()) {
            canvas.drawText("Brak danych archiwalnych", left, height / 2f, textPaint)
            return
        }
        val minValue = samples.minOf { it.minimum }
        val maxValue = samples.maxOf { it.maximum }
        val range = max(1e-9, maxValue - minValue)
        val yMin = minValue - range * 0.08
        val yMax = maxValue + range * 0.08
        val plotHeight = bottom - top
        val plotWidth = right - left
        for (tick in 0..4) {
            val y = top + plotHeight * tick / 4f
            canvas.drawLine(left, y, right, y, gridPaint)
            val value = yMax - (yMax - yMin) * tick / 4.0
            canvas.drawText(String.format(Locale.getDefault(), "%.2f", value), 2f, y + 4f * density, textPaint)
        }
        canvas.drawLine(left, top, left, bottom, axisPaint)
        canvas.drawLine(left, bottom, right, bottom, axisPaint)
        val monthLabels = arrayOf("I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII")
        for (month in 0..11) {
            val x = left + plotWidth * month / 11f
            canvas.drawText(monthLabels[month], x - 5f * density, bottom + 18f * density, textPaint)
        }
        years.entries.forEachIndexed { index, entry ->
            val color = palette[index % palette.size]
            linePaint.color = color
            val path = Path()
            entry.value.sortedBy { it.timestampStart }.forEachIndexed { pointIndex, item ->
                val x = left + plotWidth * seasonalPosition(item.timestampStart).toFloat()
                val y = bottom - (((item.average - yMin) / (yMax - yMin)) * plotHeight).toFloat()
                if (pointIndex == 0) path.moveTo(x, y) else path.lineTo(x, y)
            }
            canvas.drawPath(path, linePaint)
            val legendX = left + (index % 3) * 88f * density
            val legendY = height - 10f * density - (index / 3) * 16f * density
            canvas.drawLine(legendX, legendY - 3f * density, legendX + 18f * density, legendY - 3f * density, linePaint)
            canvas.drawText(entry.key.toString(), legendX + 23f * density, legendY, textPaint)
        }
        canvas.drawText(metricName, left, 12f * density, textPaint)
    }

    private fun utcCalendar(timestamp: Long): Calendar =
        Calendar.getInstance(TimeZone.getTimeZone("UTC")).apply { timeInMillis = timestamp * 1000L }

    private fun seasonalPosition(timestamp: Long): Double {
        val calendar = utcCalendar(timestamp)
        val month = calendar.get(Calendar.MONTH)
        val day = calendar.get(Calendar.DAY_OF_MONTH) - 1
        val daysInMonth = calendar.getActualMaximum(Calendar.DAY_OF_MONTH)
        val fractionOfDay = (
            calendar.get(Calendar.HOUR_OF_DAY) * 3600.0 +
            calendar.get(Calendar.MINUTE) * 60.0 +
            calendar.get(Calendar.SECOND)
        ) / 86400.0
        return ((month + (day + fractionOfDay) / daysInMonth) / 12.0).coerceIn(0.0, 1.0)
    }
}
