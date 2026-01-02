import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.widgets import Button, CheckButtons
import re
import numpy as np
import os

# -------------------------- 全局设置 --------------------------
def init_global_settings():
    try:
        plt.rcParams['font.sans-serif'] = ['SimHei', 'Microsoft YaHei']
    except:
        plt.rcParams['font.sans-serif'] = ['DejaVu Sans']
    plt.rcParams['axes.unicode_minus'] = False
    plt.rcParams['figure.figsize'] = (18, 14)  # 宽屏布局，适配左右分区
    plt.rcParams['font.size'] = 10

# -------------------------- 文件读取与解析 --------------------------
def read_pcode_file(file_path='pcode_output.txt'):
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"文件未找到：{os.path.abspath(file_path)}")
    
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            return f.read()
    except UnicodeDecodeError:
        with open(file_path, 'r', encoding='gbk') as f:
            return f.read()

def parse_pcode_content(content):
    instructions = []
    current_stack = []
    
    lines = [line.strip() for line in content.split('\n') if line.strip()]
    for line in lines:
        if line.startswith('newAc:') or line.startswith('back '):
            continue
        
        # 匹配指令行
        instr_match = re.match(r'^(\d+):\s+(\w+)\s+(\d+)\s+(\d+)$', line)
        if instr_match:
            pc = int(instr_match.group(1))
            op = instr_match.group(2)
            L = int(instr_match.group(3))
            A = int(instr_match.group(4))
            instructions.append({
                'pc': pc,
                'op': op,
                'L': L,
                'A': A,
                'stack': current_stack.copy()
            })
            current_stack = []
        # 匹配栈状态行
        elif line.startswith('['):
            stack_match = re.match(r'^\[(\d+)\]:\s+(.*)$', line)
            if stack_match:
                idx = int(stack_match.group(1))
                value = stack_match.group(2)
                current_stack.append((idx, value))
    
    if instructions and current_stack:
        instructions[-1]['stack'] = current_stack
    
    return instructions

# -------------------------- 解析辅助标记与过程名 --------------------------
def parse_marker_info(content, instructions):
    marker_info = [''] * len(instructions)
    procedure_names = [''] * len(instructions)
    current_marker = ''
    current_proc_name = ''
    instr_idx = 0
    
    lines = [line.strip() for line in content.split('\n') if line.strip()]
    for line in lines:
        # 提取newAc后的过程名
        newac_match = re.match(r'^newAc:(.*)$', line)
        if newac_match:
            current_proc_name = newac_match.group(1).strip()
            current_marker = line
            continue
        
        if line.startswith('back '):
            current_marker = line
            current_proc_name = ''
            continue
        
        # 关联标记与指令
        if re.match(r'^(\d+):\s+\w+\s+\d+\s+\d+$', line):
            if instr_idx < len(instructions):
                marker_info[instr_idx] = current_marker
                procedure_names[instr_idx] = current_proc_name
                current_marker = ''
                current_proc_name = ''
                instr_idx += 1
    
    return marker_info, procedure_names

# -------------------------- 可视化初始化（左右分区布局） --------------------------
def init_visualization(instructions):
    fig = plt.figure()
    fig.suptitle('Pcode 执行可视化（左栈右信息布局）', fontsize=18, fontweight='bold', y=0.96)
    
    # 左右分区：左60%显示栈，右40%显示其他信息
    ax_stack = plt.subplot(1, 2, 1)  # 左半区：栈状态（核心）
    ax_info = plt.subplot(2, 2, 2)  # 右上区：指令+过程名+辅助标记
    ax_control = plt.subplot(2, 2, 4)  # 右下区：控制选项+调用层级
    
    # -------------------------- 左半区：栈状态（核心显示） --------------------------
    ax_stack.set_title('栈状态（栈底→栈顶，显示前40项）', fontsize=16, fontweight='bold', pad=20)
    ax_stack.set_xlim(0, 2)
    ax_stack.set_ylim(-1, 40)
    ax_stack.set_xlabel('栈项', fontsize=14)
    ax_stack.set_ylabel('栈索引', fontsize=14)
    ax_stack.grid(axis='y', alpha=0.6, linestyle='--')
    ax_stack.set_facecolor('#f8f9fa')
    ax_stack.tick_params(axis='both', which='major', labelsize=12)
    
    # -------------------------- 右上区：核心信息 --------------------------
    ax_info.set_title('核心信息', fontsize=14, fontweight='bold', pad=15)
    ax_info.set_xlim(0, 1)
    ax_info.set_ylim(0, 6)
    ax_info.axis('off')
    ax_info.add_patch(plt.Rectangle((0, 0), 1, 6, facecolor='#f0f8ff', alpha=0.3))
    
    # -------------------------- 右下区：控制选项 --------------------------
    ax_control.set_title('控制选项', fontsize=14, fontweight='bold', pad=15)
    ax_control.set_xlim(0, 1)
    ax_control.set_ylim(0, 12)
    ax_control.axis('off')
    
    # 初始化文本元素
    # 右上区文本
    instr_text = ax_info.text(0.5, 5, '', ha='center', va='center', fontsize=16, fontweight='bold', color='#2c3e50')
    op_detail_text = ax_info.text(0.5, 3.8, '', ha='center', va='center', fontsize=13, color='#34495e')
    proc_name_text = ax_info.text(0.5, 2.5, '', ha='center', va='center', fontsize=14, fontweight='bold', color='#e74c3c')
    marker_text = ax_info.text(0.5, 1.2, '', ha='center', va='center', fontsize=12, color='#9b59b6')
    
    # 右下区文本
    level_text = ax_control.text(0.5, 3, '调用层级: 0\n当前过程: Main', ha='center', va='center', fontsize=13, fontweight='bold')
    
    # 控制选项（过滤按钮）
    filter_labels = ['过程调用', '过程返回', '跳转操作', '栈访问', '常量入栈', '其他指令']
    check_buttons = CheckButtons(
        ax=ax_control,
        labels=filter_labels,
        actives=[True]*6,
        frame_props=dict(edgecolor='black', linewidth=1.5),
        check_props=dict(color='green', s=60)
    )
    # 调整过滤选项位置和字体
    for i, label in enumerate(check_buttons.labels):
        label.set_fontsize(11)
        label.set_position((0.1, 0.8 - i*0.15))
    
    return fig, ax_stack, ax_info, ax_control, {
        'instr_text': instr_text,
        'op_detail_text': op_detail_text,
        'proc_name_text': proc_name_text,
        'marker_text': marker_text,
        'level_text': level_text,
        'check_buttons': check_buttons,
        'filter_labels': filter_labels
    }

# -------------------------- 可视化更新逻辑 --------------------------
def update_visualization(frame, instructions, plot_elements, call_levels, filtered_frames, marker_info, procedure_names):
    if frame >= len(filtered_frames):
        return []
    
    actual_frame = filtered_frames[frame]
    instr = instructions[actual_frame]
    stack = instr['stack']
    current_level = call_levels[actual_frame]
    current_marker = marker_info[actual_frame]
    current_proc = procedure_names[actual_frame]
    
    # -------------------------- 右上区：更新核心信息 --------------------------
    plot_elements['instr_text'].set_text(f'PC: {instr["pc"]} | 当前指令')
    plot_elements['op_detail_text'].set_text(f'操作码: {instr["op"]} | L:{instr["L"]} | A:{instr["A"]}')
    # 过程名显示
    proc_text = f'过程名: {current_proc}' if current_proc else '过程名: 无'
    plot_elements['proc_name_text'].set_text(proc_text)
    # 辅助标记显示
    marker_display = current_marker if current_marker else '无'
    plot_elements['marker_text'].set_text(f'辅助标记: {marker_display}')
    
    # -------------------------- 左半区：更新栈状态（核心） --------------------------
    ax_stack = plot_elements['instr_text'].axes.figure.axes[0]
    ax_stack.clear()
    ax_stack.set_title('栈状态（栈底→栈顶，显示前40项）', fontsize=16, fontweight='bold')
    ax_stack.set_xlim(0, 2)
    ax_stack.set_ylim(-1, 40)
    ax_stack.set_xlabel('栈项', fontsize=14)
    ax_stack.set_ylabel('栈索引', fontsize=14)
    ax_stack.grid(axis='y', alpha=0.6, linestyle='--')
    ax_stack.set_facecolor('#f8f9fa')
    ax_stack.tick_params(axis='both', which='major', labelsize=12)
    
    if stack:
        # 按索引排序，取前40项
        stack_sorted = sorted(stack, key=lambda x: x[0])[:40]
        y_positions = [item[0] for item in stack_sorted]
        values = [item[1] for item in stack_sorted]
        
        # 调整y轴范围，紧凑显示
        if y_positions:
            max_y = min(max(y_positions) + 2, 39)
            ax_stack.set_ylim(-1, max_y + 1)
        
        # 绘制栈项（优化样式）
        bars = ax_stack.bar([1]*len(y_positions), [0.9]*len(y_positions),
                          bottom=y_positions, color='#3498db', alpha=0.8, width=0.7, edgecolor='black', linewidth=1)
        
        # 栈项值标签
        for y, val in zip(y_positions, values):
            ax_stack.text(1, y + 0.45, val, ha='center', va='center', fontsize=10, fontweight='bold', color='white')
    
    # -------------------------- 右下区：更新调用层级 --------------------------
    proc_display = current_proc if current_proc else ("Main" if current_level == 0 else "Recursive")
    plot_elements['level_text'].set_text(f'调用层级: {current_level}\n当前过程: {proc_display}')
    
    return [
        plot_elements['instr_text'],
        plot_elements['op_detail_text'],
        plot_elements['proc_name_text'],
        plot_elements['marker_text'],
        plot_elements['level_text']
    ]

# -------------------------- 过滤逻辑 --------------------------
def update_filtered_frames(instructions, check_buttons):
    filtered_frames = []
    states = check_buttons.get_status()
    show_cal = states[0]
    show_opr0 = states[1]
    show_jump = states[2]
    show_stack = states[3]
    show_lit = states[4]
    show_other = states[5]
    
    for i, instr in enumerate(instructions):
        op = instr['op']
        if op == 'CAL' and show_cal:
            filtered_frames.append(i)
        elif op == 'OPR' and instr['A'] == 0 and show_opr0:
            filtered_frames.append(i)
        elif op in ['JMP', 'JPC'] and show_jump:
            filtered_frames.append(i)
        elif op in ['LOD', 'STO'] and show_stack:
            filtered_frames.append(i)
        elif op == 'LIT' and show_lit:
            filtered_frames.append(i)
        elif show_other:
            filtered_frames.append(i)
    
    return filtered_frames

# -------------------------- 辅助函数 --------------------------
def calculate_call_levels(instructions):
    call_levels = [0] * len(instructions)
    current_level = 0
    
    for i, instr in enumerate(instructions):
        if instr['op'] == 'CAL':
            current_level += 1
        elif instr['op'] == 'OPR' and instr['A'] == 0:
            current_level = max(0, current_level - 1)
        call_levels[i] = current_level
    
    return call_levels

# -------------------------- 主函数 --------------------------
def main():
    init_global_settings()
    
    try:
        pcode_content = read_pcode_file()
        instructions = parse_pcode_content(pcode_content)
        marker_info, procedure_names = parse_marker_info(pcode_content, instructions)
    except Exception as e:
        print(f"文件处理错误：{e}")
        return
    
    if not instructions:
        print("未解析到有效指令")
        return
    proc_count = sum(1 for p in procedure_names if p)
    print(f"成功解析 {len(instructions)} 条指令 + {sum(1 for m in marker_info if m)} 个辅助标记 + {proc_count} 个过程名")
    
    call_levels = calculate_call_levels(instructions)
    fig, ax_stack, ax_info, ax_control, plot_elements = init_visualization(instructions)
    
    # 初始过滤帧
    filtered_frames = update_filtered_frames(instructions, plot_elements['check_buttons'])
    max_frames = len(filtered_frames)
    
    # 过滤选项变更处理
    def on_filter_change(label):
        nonlocal filtered_frames, max_frames
        filtered_frames = update_filtered_frames(instructions, plot_elements['check_buttons'])
        max_frames = len(filtered_frames)
        # 重新创建动画
        nonlocal anim
        anim = animation.FuncAnimation(
            fig, update_visualization,
            frames=max_frames,
            fargs=(instructions, plot_elements, call_levels, filtered_frames, marker_info, procedure_names),
            interval=600,
            repeat=False,
            blit=False,
            cache_frame_data=False
        )
        anim.event_source.stop()
    
    plot_elements['check_buttons'].on_clicked(on_filter_change)
    
    # 控制按钮（添加在右下区下方）
    ax_play = plt.axes([0.65, 0.02, 0.12, 0.04])
    ax_pause = plt.axes([0.78, 0.02, 0.12, 0.04])
    ax_reset = plt.axes([0.91, 0.02, 0.12, 0.04])
    
    btn_play = Button(ax_play, '播放', color='#2ecc71', hovercolor='#27ae60')
    btn_pause = Button(ax_pause, '暂停', color='#e74c3c', hovercolor='#c0392b')
    btn_reset = Button(ax_reset, '重置', color='#f39c12', hovercolor='#d35400')
    # 按钮字体放大
    for btn in [btn_play, btn_pause, btn_reset]:
        btn.label.set_fontsize(12)
    
    # 创建动画
    anim = animation.FuncAnimation(
        fig, update_visualization,
        frames=max_frames,
        fargs=(instructions, plot_elements, call_levels, filtered_frames, marker_info, procedure_names),
        interval=600,
        repeat=False,
        blit=False,
        cache_frame_data=False
    )
    anim.event_source.stop()  # 初始暂停
    
    # 按钮事件
    def play_anim(event):
        if anim.event_source is not None:
            anim.event_source.start()
    
    def pause_anim(event):
        if anim.event_source is not None:
            anim.event_source.stop()
    
    def reset_anim(event):
        # 重置过滤选项
        for i in range(len(plot_elements['filter_labels'])):
            plot_elements['check_buttons'].set_active(i, True)
        # 重置动画
        nonlocal filtered_frames, max_frames, anim
        filtered_frames = update_filtered_frames(instructions, plot_elements['check_buttons'])
        max_frames = len(filtered_frames)
        anim = animation.FuncAnimation(
            fig, update_visualization,
            frames=max_frames,
            fargs=(instructions, plot_elements, call_levels, filtered_frames, marker_info, procedure_names),
            interval=600,
            repeat=False,
            blit=False,
            cache_frame_data=False
        )
        anim.event_source.stop()
    
    btn_play.on_clicked(play_anim)
    btn_pause.on_clicked(pause_anim)
    btn_reset.on_clicked(reset_anim)
    
    # 调整布局，避免遮挡
    plt.subplots_adjust(left=0.05, right=0.95, bottom=0.08, top=0.92, wspace=0.3, hspace=0.3)
    plt.show()

if __name__ == '__main__':
    main()