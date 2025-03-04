#================================================================================
# File: plot_csv.py                                                               
# Creator: Claudio Raimondi                                                       
# Email: claudio.raimondi@pm.me                                                   
# 
# created at: 2025-03-04 18:41:41                                                 
# last edited: 2025-03-04 18:41:41                                                
#================================================================================

import plotly.graph_objects as go
import sys

def create_cpu_cycle_chart(cpu_cycle_names, cpu_cycles):
  fig = go.Figure()

  fig.add_trace(go.Bar(x=cpu_cycle_names, y=cpu_cycles, name='CPU Cycles'))

  fig.update_layout(
    title='CPU Cycles for Different Tasks',
    xaxis_title='Task',
    yaxis_title='CPU Cycles',
    template='plotly_dark',
    showlegend=True
  )

  fig.show()

def main():
  if len(sys.argv) < 2:
    print("Usage: python cpu_cycle_chart.py <task1_name> <task1_cycles> <task2_name> <task2_cycles> ...")
    sys.exit(1)

  cpu_cycle_names = []
  cpu_cycles = []

  for i in range(1, len(sys.argv), 2):
    task_name = sys.argv[i]
    try:
      task_cycles = int(sys.argv[i + 1])
    except ValueError:
      print(f"Invalid CPU cycles value for task '{task_name}'. Please provide an integer value.")
      sys.exit(1)

    cpu_cycle_names.append(task_name)
    cpu_cycles.append(task_cycles)
  
  create_cpu_cycle_chart(cpu_cycle_names, cpu_cycles)

if __name__ == "__main__":
  main()
