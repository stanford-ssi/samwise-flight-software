import React from 'react';
import { TaskInfo } from '../types';

interface TaskListProps {
  tasks: TaskInfo[];
  selectedTasks: Set<string>;
  onToggleTask: (taskName: string) => void;
}

export const TaskList: React.FC<TaskListProps> = ({ tasks, selectedTasks, onToggleTask }) => {
  return (
    <div style={{ padding: '20px' }}>
      <h2>Discovered Tasks (click to filter)</h2>
      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(200px, 1fr))', gap: '10px' }}>
        {tasks.map((task) => {
          const isSelected = selectedTasks.has(task.name);
          return (
            <div
              key={task.name}
              onClick={() => onToggleTask(task.name)}
              style={{
                padding: '15px',
                backgroundColor: isSelected ? '#1a1a1a' : '#0a0a0a',
                borderRadius: '8px',
                borderLeft: `4px solid ${task.color}`,
                cursor: 'pointer',
                opacity: isSelected ? 1 : 0.4,
                transition: 'all 0.2s ease',
              }}
            >
              <div style={{ fontWeight: 'bold', fontSize: '14px', marginBottom: '5px' }}>
                {task.name} {isSelected ? '✓' : ''}
              </div>
              <div style={{ fontSize: '12px', color: '#aaa' }}>
                Period: {task.period_ms} ms
              </div>
              <div style={{ fontSize: '11px', color: '#666' }}>
                Index: {task.index}
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
};
