import React from 'react';
import { LogEvent } from '../types';

interface EventLogProps {
  events: LogEvent[];
  selectedTasks: Set<string>;
}

export const EventLog: React.FC<EventLogProps> = ({ events, selectedTasks }) => {
  const [showTaskLogs, setShowTaskLogs] = React.useState(true);

  // Filter events to only show selected tasks (or events without a task)
  const filteredEvents = events.filter(
    (event) => {
      // Filter by task selection
      if (event.task && !selectedTasks.has(event.task)) {
        return false;
      }
      // Optionally filter out task_log events
      if (!showTaskLogs && event.event === 'task_log') {
        return false;
      }
      return true;
    }
  );
  const getEventColor = (eventType: string) => {
    switch (eventType) {
      case 'test_start':
        return '#3b82f6';
      case 'test_pass':
        return '#10b981';
      case 'task_discovered':
        return '#8b5cf6';
      case 'task_init':
        return '#f59e0b';
      case 'task_start':
        return '#10b981';
      case 'task_end':
        return '#6b7280';
      case 'task_log':
        return '#06b6d4';
      case 'task_dispatch':
        return '#ef4444';
      case 'dispatch_cycle_start':
        return '#06b6d4';
      case 'dispatch_cycle_end':
        return '#14b8a6';
      default:
        return '#6b7280';
    }
  };

  return (
    <div style={{ padding: '20px' }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '10px' }}>
        <h2 style={{ margin: 0 }}>Event Log</h2>
        <label style={{ display: 'flex', alignItems: 'center', gap: '8px', cursor: 'pointer', fontSize: '14px' }}>
          <input
            type="checkbox"
            checked={showTaskLogs}
            onChange={(e) => setShowTaskLogs(e.target.checked)}
            style={{ cursor: 'pointer' }}
          />
          Show task logs
        </label>
      </div>
      <div
        style={{
          maxHeight: '400px',
          overflowY: 'auto',
          backgroundColor: '#1a1a1a',
          padding: '10px',
          borderRadius: '8px',
          fontFamily: 'monospace',
          fontSize: '12px',
        }}
      >
        {filteredEvents.map((event, idx) => {
          const isTaskLog = event.event === 'task_log';
          return (
            <div
              key={idx}
              style={{
                padding: '4px 8px',
                marginBottom: '2px',
                borderLeft: `3px solid ${getEventColor(event.event)}`,
                backgroundColor: isTaskLog ? '#1a2530' : '#222',
                marginLeft: isTaskLog ? '20px' : '0',
              }}
            >
              <span style={{ color: '#888' }}>[{event.time_ms}ms]</span>{' '}
              {!isTaskLog && (
                <span style={{ color: getEventColor(event.event), fontWeight: 'bold' }}>
                  {event.event}
                </span>
              )}
              {event.task && (
                <>
                  {' '}
                  <span style={{ color: '#fbbf24' }}>{event.task}</span>
                </>
              )}
              {event.details && (
                <>
                  {isTaskLog ? ': ' : ' - '}
                  <span style={{ color: '#aaa' }}>{event.details}</span>
                </>
              )}
            </div>
          );
        })}
      </div>
    </div>
  );
};
