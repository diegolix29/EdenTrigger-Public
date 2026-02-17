// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/perf_stats.h"
#include "performance_overlay.h"
#include "ui_performance_overlay.h"

#include "main_window.h"

#include <QPainterPath>

#include <QMouseEvent>
#include <QPainter>

// TODO(crueter): Reset samples when user changes turbo, slow, etc.
PerformanceOverlay::PerformanceOverlay(MainWindow* parent)
    : QWidget(parent), m_mainWindow{parent}, ui(new Ui::PerformanceOverlay) {
    ui->setupUi(this);

    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    raise();

    setMinimumHeight(160); // room for graph + labels

    // thanks Debian.
    QFont font = ui->fps->font();
    font.setWeight(QFont::DemiBold);

    ui->fps->setFont(font);
    ui->frametime->setFont(font);

    // pos/stats
    resetPosition(m_mainWindow->pos());
    connect(parent, &MainWindow::positionChanged, this, &PerformanceOverlay::resetPosition);
    connect(m_mainWindow, &MainWindow::statsUpdated, this, &PerformanceOverlay::updateStats);
}

PerformanceOverlay::~PerformanceOverlay() {
    delete ui;
}

void PerformanceOverlay::resetPosition(const QPoint& _) {
    auto pos = m_mainWindow->pos();
    move(pos.x() + m_offset.x(), pos.y() + m_offset.y());
}

void PerformanceOverlay::updateStats(const Core::PerfStatsResults& results,
                                     const VideoCore::ShaderNotify& shaders) {
    auto fps = results.average_game_fps;
    if (!std::isnan(fps)) {
        // don't sample measurements < 3 fps because they are probably outliers or freezes
        static constexpr double FPS_SAMPLE_THRESHOLD = 3.0;

        QString fpsText = tr("%1 fps").arg(std::round(fps), 0, 'f', 0);
        // if (!m_fpsSuffix.isEmpty()) fpsText = fpsText % QStringLiteral(" (%1)").arg(m_fpsSuffix);
        ui->fps->setText(fpsText);

        // sampling
        if (fps > FPS_SAMPLE_THRESHOLD) {
            m_fpsSamples.push_back(fps);
            m_fpsPoints.push_back(QPointF{m_xPos++, fps});
        }

        if (m_fpsSamples.size() > NUM_FPS_SAMPLES) {
            m_fpsSamples.pop_front();
            m_fpsPoints.pop_front();
        }

        // For the average only go back 10 samples max
        if (m_fpsSamples.size() >= 2) {
            const size_t back_search = std::min<size_t>(10, m_fpsSamples.size() - 1);
            double sum = std::accumulate(m_fpsSamples.end() - back_search, m_fpsSamples.end(), 0.0);
            double avg = sum / static_cast<double>(back_search);


            ui->fps_avg->setText(tr("Avg: %1").arg(avg, 0, 'f', 0));
        }

        update(); // repaint FPS graph
    }

    auto ft = results.frametime;
    if (!std::isnan(ft)) {
        // don't sample measurements > 500 ms because they are probably outliers
        static constexpr double FT_SAMPLE_THRESHOLD = 500.0;

        double ft_ms = results.frametime * 1000.0;
        ui->frametime->setText(tr("%1 ms").arg(ft_ms, 0, 'f', 2));

        // sampling
        if (ft_ms <= FT_SAMPLE_THRESHOLD)
            m_frametimeSamples.push_back(ft_ms);

        if (m_frametimeSamples.size() > NUM_FRAMETIME_SAMPLES)
            m_frametimeSamples.pop_front();

        if (!m_frametimeSamples.empty()) {
            auto [min_it, max_it] =
                std::minmax_element(m_frametimeSamples.begin(), m_frametimeSamples.end());
            ui->ft_min->setText(tr("Min: %1").arg(*min_it, 0, 'f', 1));
            ui->ft_max->setText(tr("Max: %1").arg(*max_it, 0, 'f', 1));
        }

        // For the average only go back 10 samples max
        if (m_frametimeSamples.size() >= 2) {
            const size_t back_search = std::min<size_t>(10, m_fpsSamples.size() - 1);
            double sum = std::accumulate(m_fpsSamples.end() - back_search, m_fpsSamples.end(), 0.0);
            double avg = sum / static_cast<double>(back_search);


            ui->ft_avg->setText(tr("Avg: %1").arg(avg, 0, 'f', 1));
        }
    }
}

void PerformanceOverlay::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // background
    painter.setBrush(m_background);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 10.0, 10.0);

    // ---- FPS GRAPH ----
    if (m_fpsSamples.size() < 2)
        return;

    const int graph_margin = 10;
    const int graph_height = 60;
    const QRect graph_rect(graph_margin, height() - graph_height - graph_margin,
                           width() - graph_margin * 2, graph_height);

    // find max FPS for scaling
    double max_fps = *std::max_element(m_fpsSamples.begin(), m_fpsSamples.end());
    max_fps = std::max(30.0, max_fps); // prevent tiny scaling

    // grid
    painter.setPen(QColor(50, 50, 50));
    painter.drawRect(graph_rect);

    // FPS line
    QPen line_pen(Qt::red);
    line_pen.setWidth(2);
    painter.setPen(line_pen);

    QPainterPath path;
    for (size_t i = 0; i < m_fpsSamples.size(); ++i) {
        const double x =
            graph_rect.left() + (double(i) / (m_fpsSamples.size() - 1)) * graph_rect.width();

        const double y = graph_rect.bottom() - (m_fpsSamples[i] / max_fps) * graph_rect.height();

        if (i == 0)
            path.moveTo(x, y);
        else
            path.lineTo(x, y);
    }

    painter.drawPath(path);
}

void PerformanceOverlay::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_drag_start_pos = event->pos();
    }
}

void PerformanceOverlay::mouseMoveEvent(QMouseEvent* event) {
    // drag
    if (event->buttons() & Qt::LeftButton) {
        QPoint new_global_pos = event->globalPosition().toPoint() - m_drag_start_pos;
        m_offset = new_global_pos - m_mainWindow->pos();
        move(new_global_pos);
    }
}

void PerformanceOverlay::closeEvent(QCloseEvent* event) {
    emit closed();
}
