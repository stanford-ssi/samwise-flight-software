import React from 'react';
import { StateSpan, StateTransition } from '../types';

interface StateMachineProps {
  stateSpans: StateSpan[];
  transitions: StateTransition[];
  maxTime: number;
}

export const StateMachine: React.FC<StateMachineProps> = ({
  stateSpans,
  transitions,
  maxTime,
}) => {
  // Get unique states in order of first appearance
  const uniqueStates: string[] = [];
  stateSpans.forEach((s) => {
    if (!uniqueStates.includes(s.name)) {
      uniqueStates.push(s.name);
    }
  });

  const lastSpan = stateSpans[stateSpans.length - 1];
  const finalState = lastSpan?.name;

  // FSM Diagram layout
  const nodeWidth = 140;
  const nodeHeight = 60;
  const nodeGap = 80;
  const diagramPadding = 40;
  const diagramWidth =
    uniqueStates.length * (nodeWidth + nodeGap) - nodeGap + diagramPadding * 2;
  const diagramHeight = nodeHeight + 80 + diagramPadding * 2;

  // State timeline bar layout
  const timelineBarHeight = 40;
  const timelineWidth = 1000;
  const timelineLeftMargin = 100;

  // Build transition counts for arrow labels
  const transitionCounts = new Map<string, number>();
  transitions.forEach((t) => {
    const key = `${t.from_state}->${t.to_state}`;
    transitionCounts.set(key, (transitionCounts.get(key) || 0) + 1);
  });

  const getStateColor = (name: string) => {
    const span = stateSpans.find((s) => s.name === name);
    return span?.color || '#666';
  };

  // Render arrow between two state nodes
  const renderArrow = (
    fromIdx: number,
    toIdx: number,
    key: string,
    count: number
  ) => {
    const fromX = diagramPadding + fromIdx * (nodeWidth + nodeGap) + nodeWidth;
    const fromY = diagramPadding + nodeHeight / 2;
    const toX = diagramPadding + toIdx * (nodeWidth + nodeGap);
    const toY = diagramPadding + nodeHeight / 2;

    if (fromIdx === toIdx) {
      // Self-loop
      const cx = diagramPadding + fromIdx * (nodeWidth + nodeGap) + nodeWidth / 2;
      const cy = diagramPadding;
      return (
        <g key={key}>
          <path
            d={`M ${cx - 15} ${cy} C ${cx - 15} ${cy - 35} ${cx + 15} ${cy - 35} ${cx + 15} ${cy}`}
            fill="none"
            stroke="#666"
            strokeWidth="1.5"
            markerEnd="url(#arrowhead)"
          />
          <text
            x={cx}
            y={cy - 30}
            textAnchor="middle"
            fontSize="10"
            fill="#aaa"
          >
            stable
          </text>
        </g>
      );
    }

    // Forward arrow
    const isSameDirection = toIdx > fromIdx;
    const curveOffset = isSameDirection ? -15 : 15;

    return (
      <g key={key}>
        <path
          d={`M ${fromX} ${fromY + curveOffset} Q ${(fromX + toX) / 2} ${fromY + curveOffset * 3} ${toX} ${toY + curveOffset}`}
          fill="none"
          stroke="#888"
          strokeWidth="1.5"
          markerEnd="url(#arrowhead)"
        />
        {count > 1 && (
          <text
            x={(fromX + toX) / 2}
            y={fromY + curveOffset * 3 - 5}
            textAnchor="middle"
            fontSize="10"
            fill="#aaa"
          >
            x{count}
          </text>
        )}
      </g>
    );
  };

  return (
    <div style={{ padding: '20px' }}>
      <h2>State Machine</h2>

      {/* FSM Diagram */}
      <div
        style={{
          backgroundColor: '#1a1a1a',
          borderRadius: '8px',
          padding: '10px',
          marginBottom: '20px',
          overflowX: 'auto',
        }}
      >
        <svg width={Math.max(diagramWidth, 400)} height={diagramHeight}>
          <defs>
            <marker
              id="arrowhead"
              markerWidth="8"
              markerHeight="6"
              refX="8"
              refY="3"
              orient="auto"
            >
              <polygon points="0 0, 8 3, 0 6" fill="#888" />
            </marker>
          </defs>

          {/* Arrows */}
          {Array.from(transitionCounts.entries()).map(([key, count]) => {
            const [from, to] = key.split('->');
            const fromIdx = uniqueStates.indexOf(from);
            const toIdx = uniqueStates.indexOf(to);
            if (fromIdx === -1 || toIdx === -1) return null;
            return renderArrow(fromIdx, toIdx, key, count);
          })}

          {/* State nodes */}
          {uniqueStates.map((name, idx) => {
            const x = diagramPadding + idx * (nodeWidth + nodeGap);
            const y = diagramPadding;
            const isFinal = name === finalState;
            const color = getStateColor(name);
            const span = stateSpans.find((s) => s.name === name);
            const taskCount = span?.tasks.length || 0;

            return (
              <g key={name}>
                {/* Outer border for final state */}
                {isFinal && (
                  <rect
                    x={x - 4}
                    y={y - 4}
                    width={nodeWidth + 8}
                    height={nodeHeight + 8}
                    rx={12}
                    ry={12}
                    fill="none"
                    stroke={color}
                    strokeWidth="2"
                    strokeDasharray="4,2"
                    opacity={0.6}
                  />
                )}
                {/* Node */}
                <rect
                  x={x}
                  y={y}
                  width={nodeWidth}
                  height={nodeHeight}
                  rx={8}
                  ry={8}
                  fill={isFinal ? color + '33' : '#222'}
                  stroke={color}
                  strokeWidth={isFinal ? 2.5 : 1.5}
                />
                {/* State name */}
                <text
                  x={x + nodeWidth / 2}
                  y={y + 25}
                  textAnchor="middle"
                  fontSize="14"
                  fontWeight="bold"
                  fill="#fff"
                >
                  {name}
                </text>
                {/* Task count */}
                <text
                  x={x + nodeWidth / 2}
                  y={y + 45}
                  textAnchor="middle"
                  fontSize="11"
                  fill="#aaa"
                >
                  {taskCount} task{taskCount !== 1 ? 's' : ''}
                </text>
              </g>
            );
          })}
        </svg>
      </div>

      {/* State Timeline Bar */}
      <div
        style={{
          backgroundColor: '#1a1a1a',
          borderRadius: '8px',
          padding: '10px',
          overflowX: 'auto',
        }}
      >
        <div
          style={{
            fontSize: '13px',
            color: '#aaa',
            marginBottom: '8px',
            fontWeight: 500,
          }}
        >
          State Timeline
        </div>
        <svg
          width={timelineWidth + timelineLeftMargin + 40}
          height={timelineBarHeight + 30}
        >
          {/* State labels + colored bars */}
          {stateSpans.map((span, idx) => {
            const endTime = span.exit_time_ms ?? maxTime;
            const x =
              timelineLeftMargin + (span.enter_time_ms / maxTime) * timelineWidth;
            const w = Math.max(
              ((endTime - span.enter_time_ms) / maxTime) * timelineWidth,
              2
            );

            return (
              <g key={idx}>
                <rect
                  x={x}
                  y={5}
                  width={w}
                  height={timelineBarHeight - 10}
                  fill={span.color}
                  opacity={0.7}
                  rx={3}
                />
                {w > 40 && (
                  <text
                    x={x + w / 2}
                    y={timelineBarHeight / 2 + 3}
                    textAnchor="middle"
                    fontSize="11"
                    fontWeight="bold"
                    fill="#fff"
                  >
                    {span.name}
                  </text>
                )}
              </g>
            );
          })}

          {/* Time markers */}
          {Array.from({ length: Math.floor(maxTime / 1000) + 1 }, (_, i) => {
            const t = i * 1000;
            const x = timelineLeftMargin + (t / maxTime) * timelineWidth;
            return (
              <g key={t}>
                <line
                  x1={x}
                  y1={timelineBarHeight - 5}
                  x2={x}
                  y2={timelineBarHeight + 5}
                  stroke="#555"
                  strokeWidth="1"
                />
                <text
                  x={x}
                  y={timelineBarHeight + 20}
                  textAnchor="middle"
                  fontSize="10"
                  fill="#666"
                >
                  {t}ms
                </text>
              </g>
            );
          })}
        </svg>
      </div>
    </div>
  );
};
