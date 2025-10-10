import React from 'react';
import { LogEvent } from '../types';

interface EventLogProps {
  events: LogEvent[];
}

export const EventLog: React.FC<EventLogProps> = ({ events }) => {
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
      <h2>Event Log</h2>
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
        {events.map((event, idx) => (
          <div
            key={idx}
            style={{
              padding: '4px 8px',
              marginBottom: '2px',
              borderLeft: `3px solid ${getEventColor(event.event)}`,
              backgroundColor: '#222',
            }}
          >
            <span style={{ color: '#888' }}>[{event.time_ms}ms]</span>{' '}
            <span style={{ color: getEventColor(event.event), fontWeight: 'bold' }}>
              {event.event}
            </span>
            {event.task && (
              <>
                {' '}
                <span style={{ color: '#fbbf24' }}>{event.task}</span>
              </>
            )}
            {event.details && (
              <>
                {' - '}
                <span style={{ color: '#aaa' }}>{event.details}</span>
              </>
            )}
          </div>
        ))}
      </div>
    </div>
  );
};
