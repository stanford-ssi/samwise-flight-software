import React from 'react';
import { LogEvent, TaskInfo } from '../types';

interface TimelineProps {
  events: LogEvent[];
  tasks: TaskInfo[];
  selectedTasks: Set<string>;
}

export const Timeline: React.FC<TimelineProps> = ({ events, tasks, selectedTasks }) => {
  // Build task execution spans (task_start to task_end pairs)
  const taskExecutions: Map<string, Array<{start: number, end: number}>> = new Map();
  const taskInits: Map<string, number> = new Map();

  events.forEach((event) => {
    if (event.event === 'task_init' && event.task) {
      taskInits.set(event.task, event.time_ms);
    } else if (event.event === 'task_start' && event.task) {
      if (!taskExecutions.has(event.task)) {
        taskExecutions.set(event.task, []);
      }
      // Find matching task_end
      const endEvent = events.find(e =>
        e.event === 'task_end' &&
        e.task === event.task &&
        e.time_ms >= event.time_ms &&
        e.time_ms <= event.time_ms + 100 // Within 100ms
      );
      if (endEvent) {
        taskExecutions.get(event.task)!.push({
          start: event.time_ms,
          end: endEvent.time_ms
        });
      }
    }
  });

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
          const isSelected = selectedTasks.has(task.name);
          const rowOpacity = isSelected ? 1 : 0.2;
          const executions = taskExecutions.get(task.name) || [];
          const initTime = taskInits.get(task.name);

          return (
            <g key={task.name} opacity={rowOpacity}>
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

              {/* Task init marker */}
              {initTime !== undefined && (
                <rect
                  x={leftMargin + (initTime / maxTime) * timelineWidth - 3}
                  y={y + 10}
                  width={6}
                  height={20}
                  fill={getTaskColor(task.name)}
                  opacity={0.6}
                />
              )}

              {/* Task execution spans */}
              {executions.map((exec, execIdx) => {
                const x1 = leftMargin + (exec.start / maxTime) * timelineWidth;
                const x2 = leftMargin + (exec.end / maxTime) * timelineWidth;
                const width = Math.max(x2 - x1, 2); // Minimum 2px width

                return (
                  <rect
                    key={execIdx}
                    x={x1}
                    y={y + 12}
                    width={width}
                    height={16}
                    fill={getTaskColor(task.name)}
                    opacity={0.7}
                    stroke={getTaskColor(task.name)}
                    strokeWidth="1"
                  />
                );
              })}
            </g>
          );
        })}
      </svg>
    </div>
  );
};
