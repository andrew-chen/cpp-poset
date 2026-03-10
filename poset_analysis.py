import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# Load the benchmark data
df = pd.read_csv('poset_benchmark_results.csv')

print("="*80)
print("POSET BENCHMARK ANALYSIS")
print("="*80)

# 1. BASIC STATISTICS
print("\n1. DATASET OVERVIEW")
print("-"*80)
print(f"Total operations recorded: {len(df)}")
print(f"\nOperations by type:")
print(df['Operation'].value_counts())
print(f"\nNode counts tested: {sorted(df['NumNodes'].unique())}")

# 2. QUERY PERFORMANCE ANALYSIS
print("\n2. QUERY PERFORMANCE (is_less_than)")
print("-"*80)
query_data = df[df['Operation'] == 'IsLessThan'].copy()
query_summary = query_data.groupby('NumNodes')['TimePerOp_NS'].agg(['mean', 'std'])
query_summary['TimePerOp_US'] = query_summary['mean'] / 1000
print(query_summary[['TimePerOp_US']])

# Linear regression to verify O(V) scaling (simple implementation)
if len(query_data) > 2:
    x = query_data['NumNodes'].values
    y = query_data['TimePerOp_NS'].values
    
    # Calculate slope and intercept manually
    x_mean = np.mean(x)
    y_mean = np.mean(y)
    slope = np.sum((x - x_mean) * (y - y_mean)) / np.sum((x - x_mean)**2)
    intercept = y_mean - slope * x_mean
    
    # Calculate R²
    y_pred = slope * x + intercept
    ss_tot = np.sum((y - y_mean)**2)
    ss_res = np.sum((y - y_pred)**2)
    r_squared = 1 - (ss_res / ss_tot)
    
    print(f"\nLinear fit: Time = {slope:.2f} * V + {intercept:.2f}")
    print(f"R² = {r_squared:.4f}")

# 3. ASSERT PERFORMANCE ANALYSIS
print("\n3. ASSERT PERFORMANCE (assert_less_than)")
print("-"*80)
assert_data = df[df['Operation'] == 'Assert'].copy()

# Compare Success vs Cycle
result_comparison = assert_data.groupby('ResultType')['TimePerOp_NS'].agg(['mean', 'median', 'std', 'count'])
result_comparison['mean_US'] = result_comparison['mean'] / 1000
result_comparison['median_US'] = result_comparison['median'] / 1000
print("\nSuccess vs Cycle Detection:")
print(result_comparison[['count', 'mean_US', 'median_US']])

# 4. SCALING BY GRAPH SIZE
print("\n4. ASSERT SCALING BY GRAPH SIZE")
print("-"*80)
for node_count in sorted(df['NumNodes'].unique()):
    subset = assert_data[assert_data['NumNodes'] == node_count]
    if len(subset) > 0:
        success = subset[subset['ResultType'] == 'Success']
        cycle = subset[subset['ResultType'] == 'Cycle']
        
        print(f"\nV = {node_count}:")
        if len(success) > 0:
            print(f"  Success operations: {len(success)}, avg time: {success['TimePerOp_NS'].mean()/1000:.2f} μs")
        if len(cycle) > 0:
            print(f"  Cycle detections: {len(cycle)}, avg time: {cycle['TimePerOp_NS'].mean()/1000:.2f} μs")

# 5. PERFORMANCE VS EDGE DENSITY
print("\n5. PERFORMANCE VS EDGE DENSITY")
print("-"*80)
for node_count in sorted(df['NumNodes'].unique()):
    subset = assert_data[(assert_data['NumNodes'] == node_count) & 
                        (assert_data['ResultType'] == 'Success')]
    if len(subset) > 10:
        # Group by edge count ranges
        subset['EdgeBin'] = pd.cut(subset['NumEdges'], bins=5)
        density_analysis = subset.groupby('EdgeBin')['TimePerOp_NS'].mean()
        print(f"\nV = {node_count}:")
        for bin_range, avg_time in density_analysis.items():
            print(f"  Edges {bin_range}: {avg_time/1000:.2f} μs avg")

# 6. GENERATE PLOTS
print("\n6. GENERATING PLOTS")
print("-"*80)

fig, axes = plt.subplots(2, 2, figsize=(14, 10))

# Plot 1: Query time vs V
ax1 = axes[0, 0]
query_summary_plot = query_data.groupby('NumNodes')['TimePerOp_NS'].mean() / 1000
ax1.plot(query_summary_plot.index, query_summary_plot.values, 'o-', linewidth=2, markersize=8)
ax1.set_xlabel('Number of Nodes (V)', fontsize=11)
ax1.set_ylabel('Query Time (μs)', fontsize=11)
ax1.set_title('Query Performance Scaling', fontsize=12, fontweight='bold')
ax1.grid(True, alpha=0.3)

# Plot 2: Assert time distribution by result type
ax2 = axes[0, 1]
success_times = assert_data[assert_data['ResultType'] == 'Success']['TimePerOp_NS'] / 1000
cycle_times = assert_data[assert_data['ResultType'] == 'Cycle']['TimePerOp_NS'] / 1000
ax2.hist([success_times, cycle_times], bins=50, label=['Success', 'Cycle'], alpha=0.7)
ax2.set_xlabel('Time (μs)', fontsize=11)
ax2.set_ylabel('Frequency', fontsize=11)
ax2.set_title('Assert Operation Time Distribution', fontsize=12, fontweight='bold')
ax2.legend()
ax2.set_xlim(0, np.percentile(assert_data['TimePerOp_NS']/1000, 95))
ax2.grid(True, alpha=0.3)

# Plot 3: Performance degradation with edge density (V=5000)
ax3 = axes[1, 0]
largest_graph = assert_data[assert_data['NumNodes'] == max(df['NumNodes'].unique())]
largest_success = largest_graph[largest_graph['ResultType'] == 'Success']
if len(largest_success) > 0:
    ax3.scatter(largest_success['NumEdges'], largest_success['TimePerOp_NS']/1000, 
               alpha=0.5, s=20)
    ax3.set_xlabel('Number of Edges (E)', fontsize=11)
    ax3.set_ylabel('Assert Time (μs)', fontsize=11)
    ax3.set_title(f'Performance vs Density (V={max(df["NumNodes"].unique())})', 
                 fontsize=12, fontweight='bold')
    ax3.grid(True, alpha=0.3)

# Plot 4: Average assert time by graph size
ax4 = axes[1, 1]
size_performance = assert_data.groupby('NumNodes')['TimePerOp_NS'].mean() / 1000
ax4.bar(size_performance.index.astype(str), size_performance.values, alpha=0.7, color='steelblue')
ax4.set_xlabel('Number of Nodes (V)', fontsize=11)
ax4.set_ylabel('Average Assert Time (μs)', fontsize=11)
ax4.set_title('Average Assert Performance by Graph Size', fontsize=12, fontweight='bold')
ax4.grid(True, alpha=0.3, axis='y')

plt.tight_layout()
plt.savefig('poset_benchmark_analysis.png', dpi=300, bbox_inches='tight')
print("Saved: poset_benchmark_analysis.png")

# 7. SUMMARY STATISTICS FOR PAPER
print("\n7. KEY STATISTICS FOR PAPER")
print("="*80)

print("\nQuery Performance Summary:")
print(f"  Range: {query_data['TimePerOp_NS'].min()/1000:.1f} - {query_data['TimePerOp_NS'].max()/1000:.1f} μs")
print(f"  Scaling factor (5000/100 nodes): {query_data[query_data['NumNodes']==5000]['TimePerOp_NS'].values[0] / query_data[query_data['NumNodes']==100]['TimePerOp_NS'].values[0]:.1f}x")

print("\nAssert Performance Summary:")
print(f"  Success operations:")
print(f"    Median: {success_times.median():.1f} μs")
print(f"    95th percentile: {success_times.quantile(0.95):.1f} μs")
print(f"  Cycle detections:")
print(f"    Median: {cycle_times.median():.1f} μs")
print(f"    95th percentile: {cycle_times.quantile(0.95):.1f} μs")

print("\nCycle Detection Rate:")
for node_count in sorted(df['NumNodes'].unique()):
    subset = assert_data[assert_data['NumNodes'] == node_count]
    if len(subset) > 0:
        cycle_rate = len(subset[subset['ResultType'] == 'Cycle']) / len(subset) * 100
        print(f"  V={node_count}: {cycle_rate:.1f}% cycles detected")

print("\n" + "="*80)
print("Analysis complete. Review poset_benchmark_analysis.png for visualizations.")
print("="*80)