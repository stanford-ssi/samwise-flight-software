import React from 'react';
import { LogEvent, TaskInfo } from '../types';

interface TimelineProps {
  events: LogEvent[];
  tasks: TaskInfo[];
}

export const Timeline: React.FC<TimelineProps> = ({ events, tasks }) => {
  const taskEvents = events.filter(e =>
    e.event === 'task_dispatch' || e.event === 'task_init'
  );

  const maxTime = Math.max(...events.map(e => e.time_ms), 1000);
  const timelineWidth = 1200;
  const rowHeight = 40;
  const leftMargin = 150;

  const getTaskColor = (taskName: string) => {
    const task = tasks.find(t => t.name === taskName);
    return task?.color || '#999';
  };

  const renderTimeMarkers = () => {
    const markers = [];
    const step = 1000; // 1 second markers
    for (let t = 0; t <= maxTime; t += step) {
      const x = leftMargin + (t / maxTime) * timelineWidth;
      markers.push(
        <g key={t}>
          <line
            x1={x}
            y1={0}
            x2={x}
            y2={(tasks.length + 1) * rowHeight}
            stroke="#333"
            strokeWidth="1"
            strokeDasharray="2,2"
          />
          <text
            x={x}
            y={(tasks.length + 1) * rowHeight + 15}
            textAnchor="middle"
            fontSize="10"
            fill="#666"
          >
            {t}ms
          </text>
        </g>
      );
    }
    return markers;
  };

  return (
    <div style={{ padding: '20px', overflowX: 'auto' }}>
      <h2>Task Execution Timeline</h2>
      <svg
        width={timelineWidth + leftMargin + 50}
        height={(tasks.length + 2) * rowHeight + 30}
      >
        {/* Time markers */}
        {renderTimeMarkers()}

        {/* Task rows */}
        {tasks.map((task, idx) => {
          const y = (idx + 1) * rowHeight;
          const taskEvs = taskEvents.filter(e => e.task === task.name);

          return (
            <g key={task.name}>
              {/* Task label */}
              <text
                x={10}
                y={y + 20}
                fontSize="12"
                fontWeight="bold"
                fill="#fff"
              >
                {task.name}
              </text>
              <text
                x={10}
                y={y + 32}
                fontSize="9"
                fill="#aaa"
              >
                {task.period_ms}ms
              </text>

              {/* Horizontal line */}
              <line
                x1={leftMargin}
                y1={y + 20}
                x2={leftMargin + timelineWidth}
                y2={y + 20}
                stroke="#444"
                strokeWidth="1"
              />

              {/* Task events */}
              {taskEvs.map((event, eventIdx) => {
                const x = leftMargin + (event.time_ms / maxTime) * timelineWidth;
                const isInit = event.event === 'task_init';

                return (
                  <g key={eventIdx}>
                    <rect
                      x={x - 3}
                      y={y + 10}
                      width={isInit ? 6 : 8}
                      height={isInit ? 20 : 20}
                      fill={getTaskColor(task.name)}
                      opacity={isInit ? 0.6 : 0.9}
                    />
                    {!isInit && (
                      <rect
                        x={x - 3}
                        y={y + 10}
                        width={8}
                        height={20}
                        fill="none"
                        stroke="#fff"
                        strokeWidth="1"
                      />
                    )}
                  </g>
                );
              })}
            </g>
          );
        })}
      </svg>
    </div>
  );
};
