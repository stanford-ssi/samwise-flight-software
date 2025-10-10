import React from 'react';
import { TaskInfo } from '../types';

interface TaskListProps {
  tasks: TaskInfo[];
}

export const TaskList: React.FC<TaskListProps> = ({ tasks }) => {
  return (
    <div style={{ padding: '20px' }}>
      <h2>Discovered Tasks</h2>
      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(200px, 1fr))', gap: '10px' }}>
        {tasks.map((task) => (
          <div
            key={task.name}
            style={{
              padding: '15px',
              backgroundColor: '#1a1a1a',
              borderRadius: '8px',
              borderLeft: `4px solid ${task.color}`,
            }}
          >
            <div style={{ fontWeight: 'bold', fontSize: '14px', marginBottom: '5px' }}>
              {task.name}
            </div>
            <div style={{ fontSize: '12px', color: '#aaa' }}>
              Period: {task.period_ms} ms
            </div>
            <div style={{ fontSize: '11px', color: '#666' }}>
              Index: {task.index}
            </div>
          </div>
        ))}
      </div>
    </div>
  );
};
