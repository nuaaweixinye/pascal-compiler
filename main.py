import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.widgets import Button, CheckButtons
import re
import numpy as np
import os

# -------------------------- 全局设置 & 全局变量 --------------------------
def init_global_settings():
    try:
        plt.rcParams['font.sans-serif'] = ['SimHei', 'Microsoft YaHei']
    except:
        plt.rcParams['font.sans-serif'] = ['DejaVu Sans']
    plt.rcParams['axes.unicode_minus'] = False
    plt.rcParams['figure.figsize'] = (18, 14)
    plt.rcParams['font.size'] = 10
    plt.rcParams['axes.grid'] = False
    plt.rcParams['figure.facecolor'] = 'white'
    plt.rcParams['savefig.facecolor'] = 'white'

# 全局变量
anim = None
fig = None
filtered_frames = []
max_frames = 0
plot_elements = None
instructions = None
call_levels = None
marker_info = None
procedure_names = None

# -------------------------- 文件读取与解析（精准匹配你的日志格式） --------------------------
def read_pcode_file(file_path='pcode_output.txt'):
    """读取Pcode日志文件，自动适配utf-8/gbk编码"""
    file_abs_path = os.path.abspath(file_path)
    if not os.path.exists(file_abs_path):
        raise FileNotFoundError(f"Pcode日志文件未找到：{file_abs_path}")
    
    try:
        with open(file_abs_path, 'r', encoding='utf-8') as f:
            content = f.read()
        return content
    except UnicodeDecodeError:
        try:
            with open(file_abs_path, 'r', encoding='gbk') as f:
                content = f.read()
            return content
        except Exception as e:
            raise Exception(f"文件编码不支持，错误信息：{e}")

def parse_pcode_content(content):
    """
    精准适配你的日志格式：
    1.  指令格式：数字: 操作码 数字 数字（如 0: JMP 0 37）
    2.  栈格式：每行独立 [索引]: 值（如 [7]: 0）
    3.  逻辑：解析到指令后，持续收集后续栈行，直到下一条指令出现
    """
    instructions_list = []
    current_stack = []  # 收集当前指令对应的栈数据
    lines = [line.strip() for line in content.split('\n') if line.strip()]

    for line in lines:
        # 1. 匹配指令行（优先处理指令，因为指令是栈数据的分隔符）
        instr_match = re.match(r'^(\d+):\s+(\w+)\s+(-?\d+)\s+(-?\d+)$', line)
        if instr_match:
            # 如果当前有未关联的栈数据（上一条指令的栈），先忽略（这里只关联当前指令的栈）
            # 提取当前指令信息
            pc = int(instr_match.group(1))
            op = instr_match.group(2)
            L = int(instr_match.group(3))
            A = int(instr_match.group(4))

            # 先创建指令对象，暂时关联空栈（后续会补充）
            instr_obj = {
                'pc': pc,
                'op': op,
                'L': L,
                'A': A,
                'stack': []
            }
            instructions_list.append(instr_obj)

            # 关键：当前指令出现，说明上一条指令的栈数据收集完毕
            # 但这里先重置current_stack，准备收集当前指令的栈数据
            # （因为日志中是 指令 → 多行栈 → 下一条指令，所以当前指令后的栈才是它的）
            # 先把current_stack赋值给上一条指令（如果存在）
            if len(instructions_list) >= 2 and current_stack:
                instructions_list[-2]['stack'] = current_stack.copy()
            # 重置栈收集器，准备收集当前指令的栈
            current_stack = []
            continue

        # 2. 匹配栈行（你的格式：[索引]: 值，精准匹配）
        stack_match = re.match(r'^\[(\d+)\]\s*:\s*(.*?)\s*$', line)
        if stack_match:
            idx = int(stack_match.group(1))
            val = stack_match.group(2).strip()
            current_stack.append((idx, val))
            continue

        # 3. 跳过过程标记（newAc/back），同时处理栈数据
        if line.startswith('newAc:') or line.startswith('back '):
            # 如果有未关联的栈数据，关联到最后一条指令
            if current_stack and instructions_list:
                instructions_list[-1]['stack'] = current_stack.copy()
            current_stack = []
            continue

    # 处理最后一条指令的栈数据（文件末尾的栈）
    if current_stack and instructions_list:
        instructions_list[-1]['stack'] = current_stack.copy()

    # 确保没有空栈（兜底，防止个别指令无栈）
    for instr in instructions_list:
        if not instr['stack'] or len(instr['stack']) == 0:
            instr['stack'] = [(0, f"{instr['op']}_默认值")]

    return instructions_list

# -------------------------- 解析辅助标记与过程名 --------------------------
def parse_marker_info(content, instructions_list):
    marker_info_list = [''] * len(instructions_list)
    procedure_names_list = [''] * len(instructions_list)
    current_marker = ''
    current_proc_name = ''
    instr_idx = 0
    
    lines = [line.strip() for line in content.split('\n') if line.strip()]
    for line in lines:
        newac_match = re.match(r'^newAc:(.*)$', line)
        if newac_match:
            current_proc_name = newac_match.group(1).strip()
            current_marker = line
            continue
        
        if line.startswith('back '):
            current_marker = line
            current_proc_name = ''
            continue
        
        if re.match(r'^(\d+):\s+\w+\s+\d+\s+\d+$', line):
            if instr_idx < len(instructions_list):
                marker_info_list[instr_idx] = current_marker
                procedure_names_list[instr_idx] = current_proc_name
                current_marker = ''
                current_proc_name = ''
                instr_idx += 1
    
    return marker_info_list, procedure_names_list

# -------------------------- 可视化初始化 --------------------------
def init_visualization(instructions_list):
    global fig
    fig = plt.figure()
    fig.suptitle('Pcode 执行动态可视化（匹配你的日志格式）', fontsize=18, fontweight='bold', y=0.96)
    
    ax_stack = plt.subplot(1, 2, 1)
    ax_info = plt.subplot(2, 2, 2)
    ax_control = plt.subplot(2, 2, 4)
    
    # 左半区：栈状态
    ax_stack.set_title('运行时栈状态（栈底 → 栈顶）', fontsize=16, fontweight='bold', pad=20)
    ax_stack.set_xlim(0, 2)
    ax_stack.set_ylim(-1, 10)  # 适配你的栈索引（0-8）
    ax_stack.set_xlabel('栈项内容', fontsize=14, fontweight='bold')
    ax_stack.set_ylabel('栈索引', fontsize=14, fontweight='bold')
    ax_stack.grid(axis='y', alpha=0.6, linestyle='--', linewidth=0.8)
    ax_stack.set_facecolor('#f8f9fa')
    ax_stack.tick_params(axis='both', which='major', labelsize=12)
    
    # 右上区：核心信息
    ax_info.set_title('指令核心信息', fontsize=14, fontweight='bold', pad=15)
    ax_info.set_xlim(0, 1)
    ax_info.set_ylim(0, 6)
    ax_info.axis('off')
    ax_info.add_patch(plt.Rectangle((0, 0), 1, 6, facecolor='#f0f8ff', alpha=0.5, edgecolor='#bcdff1', linewidth=2))
    
    # 右下区：控制选项
    ax_control.set_title('指令过滤选项', fontsize=14, fontweight='bold', pad=15)
    ax_control.set_xlim(0, 1)
    ax_control.set_ylim(0, 12)
    ax_control.axis('off')
    
    # 文本元素
    instr_text = ax_info.text(0.5, 5, '', ha='center', va='center', fontsize=16, fontweight='bold', color='#2c3e50')
    op_detail_text = ax_info.text(0.5, 3.8, '', ha='center', va='center', fontsize=13, color='#34495e')
    proc_name_text = ax_info.text(0.5, 2.5, '', ha='center', va='center', fontsize=14, fontweight='bold', color='#e74c3c')
    marker_text = ax_info.text(0.5, 1.2, '', ha='center', va='center', fontsize=12, color='#9b59b6')
    level_text = ax_control.text(0.5, 3, '调用层级: 0\n当前过程: Main', ha='center', va='center', fontsize=13, fontweight='bold')
    
    # 过滤复选框
    filter_labels = ['过程调用', '过程返回', '跳转操作', '栈访问', '常量入栈', '其他指令']
    check_buttons = CheckButtons(
        ax=ax_control,
        labels=filter_labels,
        actives=[True]*6,
        frame_props=dict(edgecolor='black', linewidth=1.5, facecolor='#f8f9fa'),
        check_props=dict(color='green', s=60)
    )
    for i, label in enumerate(check_buttons.labels):
        label.set_fontsize(11)
        label.set_position((0.1, 0.8 - i*0.15))
        label.set_color('#2c3e50')
    
    global plot_elements
    plot_elements = {
        'instr_text': instr_text,
        'op_detail_text': op_detail_text,
        'proc_name_text': proc_name_text,
        'marker_text': marker_text,
        'level_text': level_text,
        'check_buttons': check_buttons,
        'filter_labels': filter_labels
    }
    
    return ax_stack, ax_info, ax_control

# -------------------------- 可视化更新逻辑（精准绘制你的栈数据） --------------------------
def update_visualization(frame):
    if frame >= len(filtered_frames):
        return []
    
    actual_frame = filtered_frames[frame]
    instr = instructions[actual_frame]
    stack = instr['stack']
    current_level = call_levels[actual_frame]
    current_marker = marker_info[actual_frame]
    current_proc = procedure_names[actual_frame]
    
    # 更新右上区信息
    plot_elements['instr_text'].set_text(f'程序计数器(PC): {instr["pc"]} | 当前执行指令')
    plot_elements['op_detail_text'].set_text(f'操作码(OP): {instr["op"]} | 层差(L): {instr["L"]} | 位移量(A): {instr["A"]}')
    proc_text = f'当前过程: {current_proc}' if current_proc else '当前过程: Main（全局）'
    plot_elements['proc_name_text'].set_text(proc_text)
    marker_display = current_marker if current_marker else '无辅助标记'
    plot_elements['marker_text'].set_text(f'状态标记: {marker_display}')
    
    # 更新左半区栈状态
    ax_stack = fig.axes[0]
    ax_stack.clear()
    ax_stack.set_title('运行时栈状态（栈底 → 栈顶）', fontsize=16, fontweight='bold', pad=20)
    ax_stack.set_xlim(0, 2)
    ax_stack.set_ylim(-1, 10)  # 适配你的栈索引范围
    ax_stack.set_xlabel('栈项内容', fontsize=14, fontweight='bold')
    ax_stack.set_ylabel('栈索引', fontsize=14, fontweight='bold')
    ax_stack.grid(axis='y', alpha=0.6, linestyle='--', linewidth=0.8)
    ax_stack.set_facecolor('#f8f9fa')
    ax_stack.tick_params(axis='both', which='major', labelsize=12)
    
    # 绘制栈状态（按你的栈索引排序，栈底=小索引，栈顶=大索引）
    if stack and len(stack) > 0:
        # 按栈索引升序排序（确保栈底在下，栈顶在上）
        stack_sorted = sorted(stack, key=lambda x: x[0])
        y_positions = [item[0] for item in stack_sorted]
        values = [item[1] for item in stack_sorted]
        
        # 动态调整y轴范围（适配你的栈索引）
        if y_positions:
            min_y = min(y_positions) - 1
            max_y = max(y_positions) + 1
            ax_stack.set_ylim(min_y, max_y)
        
        # 绘制栈项柱状图
        bars = ax_stack.bar(
            [1]*len(y_positions), 
            [0.9]*len(y_positions),
            bottom=y_positions, 
            color='#3498db', 
            alpha=0.8, 
            width=0.7, 
            edgecolor='black', 
            linewidth=1
        )
        
        # 添加栈项值标签（适配你的栈值格式）
        for y, val in zip(y_positions, values):
            ax_stack.text(1, y + 0.45, val, ha='center', va='center', fontsize=10, fontweight='bold', color='white')
    else:
        ax_stack.text(1, 5, '当前无栈数据', ha='center', va='center', fontsize=14, fontweight='bold', color='#e74c3c')
    
    # 更新右下区调用层级
    proc_display = current_proc if current_proc else ("Main（全局）" if current_level == 0 else "递归过程")
    plot_elements['level_text'].set_text(f'当前调用层级: {current_level}\n当前活跃过程: {proc_display}')
    
    return [
        plot_elements['instr_text'],
        plot_elements['op_detail_text'],
        plot_elements['proc_name_text'],
        plot_elements['marker_text'],
        plot_elements['level_text']
    ]

# -------------------------- 过滤逻辑 --------------------------
def update_filtered_frames():
    global filtered_frames, max_frames
    filtered_frames = []
    states = plot_elements['check_buttons'].get_status()
    show_cal = states[0]
    show_opr0 = states[1]
    show_jump = states[2]
    show_stack = states[3]
    show_lit = states[4]
    show_other = states[5]
    
    for i, instr in enumerate(instructions):
        op = instr['op']
        a_val = instr['A']
        if (op == 'CAL' and show_cal) or \
           (op == 'OPR' and a_val == 0 and show_opr0) or \
           (op in ['JMP', 'JPC'] and show_jump) or \
           (op in ['LOD', 'STO'] and show_stack) or \
           (op == 'LIT' and show_lit) or \
           (show_other and op not in ['CAL', 'OPR', 'JMP', 'JPC', 'LOD', 'STO', 'LIT']):
            filtered_frames.append(i)
    max_frames = len(filtered_frames)

# -------------------------- 按钮事件回调 --------------------------
def on_filter_change(label):
    update_filtered_frames()
    global anim
    anim = animation.FuncAnimation(
        fig, update_visualization,
        frames=max_frames,
        interval=600,
        repeat=False,
        blit=False,
        cache_frame_data=False
    )
    anim.event_source.stop()

def play_anim(event):
    global anim
    if anim and anim.event_source is not None:
        anim.event_source.start()
        print("🎬 动画开始播放...")

def pause_anim(event):
    global anim
    if anim and anim.event_source is not None:
        anim.event_source.stop()
        print("⏸️  动画已暂停")

def reset_anim(event):
    global anim
    for i in range(len(plot_elements['filter_labels'])):
        plot_elements['check_buttons'].set_active(i, True)
    update_filtered_frames()
    anim = animation.FuncAnimation(
        fig, update_visualization,
        frames=max_frames,
        interval=600,
        repeat=False,
        blit=False,
        cache_frame_data=False
    )
    anim.event_source.stop()
    print("🔄 动画已重置（过滤选项恢复默认）")

# -------------------------- 计算调用层级 --------------------------
def calculate_call_levels(instructions_list):
    call_levels_list = [0] * len(instructions_list)
    current_level = 0
    
    for i, instr in enumerate(instructions_list):
        op = instr['op']
        a_val = instr['A']
        if op == 'CAL':
            current_level += 1
        elif op == 'OPR' and a_val == 0:
            current_level = max(0, current_level - 1)
        call_levels_list[i] = current_level
    
    return call_levels_list

# -------------------------- 主函数 --------------------------
def main():
    global anim, instructions, call_levels, marker_info, procedure_names
    init_global_settings()
    
    try:
        pcode_content = read_pcode_file()
        instructions = parse_pcode_content(pcode_content)
        marker_info, procedure_names = parse_marker_info(pcode_content, instructions)
    except Exception as e:
        print(f"文件读取/解析失败：{e}")
        return
    
    if not instructions:
        print("未解析到有效Pcode指令，请检查日志文件格式")
        return
    
    # 统计解析结果（验证栈数据是否解析成功）
    marker_count = sum(1 for m in marker_info if m)
    proc_count = sum(1 for p in procedure_names if p)
    stack_data_count = sum(1 for instr in instructions if len(instr['stack']) > 0 and instr['stack'][0][1] != f"{instr['op']}_默认值")
    print(f"✅ 解析成功：{len(instructions)} 条指令 | {marker_count} 个状态标记 | {proc_count} 个过程名 | {stack_data_count} 条有效栈数据")
    
    call_levels = calculate_call_levels(instructions)
    ax_stack, ax_info, ax_control = init_visualization(instructions)
    
    update_filtered_frames()
    plot_elements['check_buttons'].on_clicked(on_filter_change)
    
    # 创建按钮
    ax_play = plt.axes([0.65, 0.02, 0.12, 0.04])
    ax_pause = plt.axes([0.78, 0.02, 0.12, 0.04])
    ax_reset = plt.axes([0.91, 0.02, 0.12, 0.04])
    
    btn_play = Button(ax_play, '播放', color='#2ecc71', hovercolor='#27ae60')
    btn_pause = Button(ax_pause, '暂停', color='#e74c3c', hovercolor='#c0392b')
    btn_reset = Button(ax_reset, '重置', color='#f39c12', hovercolor='#d35400')
    
    for btn in [btn_play, btn_pause, btn_reset]:
        btn.label.set_fontsize(12)
        btn.label.set_fontweight('bold')
    
    # 绑定事件
    btn_play.on_clicked(play_anim)
    btn_pause.on_clicked(pause_anim)
    btn_reset.on_clicked(reset_anim)
    
    # 创建动画
    anim = animation.FuncAnimation(
        fig, update_visualization,
        frames=max_frames,
        interval=600,
        repeat=False,
        blit=False,
        cache_frame_data=False
    )
    anim.event_source.stop()
    
    plt.subplots_adjust(left=0.05, right=0.95, bottom=0.08, top=0.92, wspace=0.3, hspace=0.3)
    plt.show()

if __name__ == '__main__':
    main()