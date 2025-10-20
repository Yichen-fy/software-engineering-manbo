import os
import subprocess
import json
import sys
from datetime import datetime

def compare_json_with_rules(dump_data, expect_data, path=""):
    """
    按照规则比较两个JSON对象，只关注expect中存在的元素
    :param dump_data: 生成的JSON数据
    :param expect_data: 期望的JSON数据
    :param path: 当前比较的路径（用于错误信息）
    :return: 差异信息列表
    """
    differences = []
    
    # 处理不同类型的情况
    if type(expect_data) != type(dump_data):
        differences.append(f"类型不同在 {path}: expect是{type(expect_data).__name__}, dump是{type(dump_data).__name__}")
        return differences
    
    # 检查是否为财神位置字段（改进的路径匹配逻辑）
    if "god" in path and "location" in path:
        # 使用更宽松的匹配条件来识别财神位置字段
        path_parts = path.split('.')
        for i in range(len(path_parts) - 1):
            if path_parts[i] == "god" and path_parts[i + 1] == "location":
                return differences  # 跳过财神位置的比较
    
    # 新增：跳过炸弹相关的比较
    if "bomb" in path.lower():
        # 检查路径是否包含bomb关键词
        path_parts = path.split('.')
        for part in path_parts:
            if "bomb" in part.lower():
                return differences  # 跳过炸弹的比较
    
    if isinstance(expect_data, dict):
        # 对于字典，只检查expect中存在的键
        for key in expect_data:
            # 跳过炸弹相关的键
            if "bomb" in key.lower():
                continue
                
            new_path = f"{path}.{key}" if path else key
            if key not in dump_data:
                differences.append(f"键缺失在 {new_path}: dump中缺少'{key}'")
            else:
                differences.extend(compare_json_with_rules(dump_data[key], expect_data[key], new_path))
                
    elif isinstance(expect_data, list):
        # 对于数组，需要特殊处理
        if not expect_data:  # 空数组
            if dump_data:
                differences.append(f"数组长度不同在 {path}: expect为空, dump有{len(dump_data)}个元素")
            return differences
            
        # 检查第一个元素，确定数组类型
        first_expect = expect_data[0]
        
        if isinstance(first_expect, dict):
            # 对于对象数组，尝试按照特定规则匹配元素
            if "index" in first_expect:
                # 按照index匹配（如players）
                expect_by_index = {item["index"]: item for item in expect_data}
                dump_by_index = {item["index"]: item for item in dump_data}
                
                for index, expect_item in expect_by_index.items():
                    new_path = f"{path}[index={index}]"
                    if index not in dump_by_index:
                        differences.append(f"元素缺失在 {new_path}: dump中缺少index为{index}的元素")
                    else:
                        differences.extend(compare_json_with_rules(dump_by_index[index], expect_item, new_path))
                        
            elif "location" in first_expect and path.endswith(("bomb", "barrier")):
                # 按照location匹配（如bomb和barrier）
                # 跳过炸弹数组的比较
                if "bomb" in path.lower():
                    return differences
                    
                expect_by_location = {item["location"]: item for item in expect_data}
                dump_by_location = {item["location"]: item for item in dump_data}
                
                for location, expect_item in expect_by_location.items():
                    new_path = f"{path}[location={location}]"
                    if location not in dump_by_location:
                        differences.append(f"元素缺失在 {new_path}: dump中缺少location为{location}的元素")
                    else:
                        differences.extend(compare_json_with_rules(dump_by_location[location], expect_item, new_path))
            else:
                # 默认按顺序匹配
                for i in range(min(len(expect_data), len(dump_data))):
                    differences.extend(compare_json_with_rules(dump_data[i], expect_data[i], f"{path}[{i}]"))
                
                if len(expect_data) > len(dump_data):
                    differences.append(f"数组长度不同在 {path}: expect有{len(expect_data)}元素, dump只有{len(dump_data)}元素")
        else:
            # 对于基本类型数组，按顺序比较
            # 跳过炸弹数组
            if "bomb" in path.lower():
                return differences
                
            # 新增：对于barrier数组，忽略顺序比较
            if "barrier" in path.lower():
                # 将数组转换为集合进行比较（忽略顺序和重复元素）
                expect_set = set(expect_data)
                dump_set = set(dump_data)
                
                # 检查缺失的元素
                missing_elements = expect_set - dump_set
                if missing_elements:
                    differences.append(f"barrier数组元素缺失在 {path}: dump缺少元素 {sorted(missing_elements)}")
                
                # 检查多余的元素（可选，根据需求决定是否报告）
                extra_elements = dump_set - expect_set
                if extra_elements:
                    differences.append(f"barrier数组多余元素在 {path}: dump有额外元素 {sorted(extra_elements)}")
                
                return differences
                
            # 对于其他基本类型数组，按顺序比较
            for i in range(min(len(expect_data), len(dump_data))):
                if expect_data[i] != dump_data[i]:
                    differences.append(f"值不同在 {path}[{i}]: expect是{expect_data[i]}, dump是{dump_data[i]}")
            
            if len(expect_data) > len(dump_data):
                differences.append(f"数组长度不同在 {path}: expect有{len(expect_data)}元素, dump只有{len(dump_data)}元素")
                
    else:
        # 基本类型值比较
        if expect_data != dump_data:
            differences.append(f"值不同在 {path}: expect是{expect_data}, dump是{dump_data}")
    
    return differences

def compare_json_files(dump_path, expect_path):
    """
    比较两个JSON文件的内容，使用规则进行比较
    :param dump_path: 生成的dump.json文件路径
    :param expect_path: 期望的expect.json文件路径
    :return: 差异信息字符串
    """
    try:
        with open(dump_path, 'r', encoding='utf-8') as dump_file:
            dump_data = json.load(dump_file)
        with open(expect_path, 'r', encoding='utf-8') as expect_file:
            expect_data = json.load(expect_file)
    except json.JSONDecodeError as e:
        return f"JSON解析错误: {e}"
    
    differences = compare_json_with_rules(dump_data, expect_data)
    
    if differences:
        return "发现差异:\n" + "\n".join(differences)
    else:
        return "无差异"

def get_expect_file_path(test_dir):
    """
    获取期望文件的路径，优先使用expect.json，如果不存在则使用expected_result.json
    :param test_dir: 测试目录路径
    :return: 期望文件路径
    """
    expect_path = os.path.join(test_dir, "expect.json")
    if os.path.exists(expect_path):
        return expect_path
    
    expected_result_path = os.path.join(test_dir, "expected_result.json")
    if os.path.exists(expected_result_path):
        return expected_result_path
    
    # 如果两个文件都不存在，返回默认的expect.json路径（用于错误提示）
    return expect_path

def run_test(test_dir, executable, result_file):
    """
    运行单个测试样例
    :param test_dir: 测试目录路径
    :param executable: 可执行文件路径
    :param result_file: 结果文件对象
    :return: 测试是否通过（布尔值）
    """
    # 检查测试目录是否存在
    if not os.path.isdir(test_dir):
        error_msg = f"测试目录不存在: {test_dir}"
        print(error_msg)
        result_file.write(error_msg + "\n")
        return False

    test_txt_path = os.path.join(test_dir, "input.txt")
    expect_path = get_expect_file_path(test_dir)
    dump_path = os.path.join(test_dir, "dump.json")

    # 检查必要的文件是否存在
    if not os.path.exists(test_txt_path):
        error_msg = f"缺少test.txt文件: {test_txt_path}"
        print(error_msg)
        result_file.write(error_msg + "\n")
        return False
    if not os.path.exists(expect_path):
        error_msg = f"缺少expect.json文件: {expect_path}"
        print(error_msg)
        result_file.write(error_msg + "\n")
        return False

    # 读取测试命令
    with open(test_txt_path, 'r', encoding='utf-8') as f:
        input_commands = f.read()

    # 运行程序，将测试目录作为参数传递，并将test.txt内容作为标准输入
    process = subprocess.Popen([executable, test_dir], 
                               stdin=subprocess.PIPE, 
                               stdout=subprocess.PIPE, 
                               stderr=subprocess.PIPE)
    stdout, stderr = process.communicate(input=input_commands.encode('utf-8'))

    # 检查程序是否成功运行
    if process.returncode != 0:
        error_msg = f"程序运行失败，返回码：{process.returncode}"
        print(error_msg)
        result_file.write(error_msg + "\n")
        error_msg = "标准错误输出："
        print(error_msg)
        result_file.write(error_msg + "\n")
        stderr_msg = stderr.decode('utf-8', errors='replace')
        print(stderr_msg)
        result_file.write(stderr_msg + "\n")
        return False

    # 检查dump.json是否生成
    if not os.path.exists(dump_path):
        error_msg = "dump.json未生成"
        print(error_msg)
        result_file.write(error_msg + "\n")
        return False

    # 比较dump.json和expect.json的内容
    diff_result = compare_json_files(dump_path, expect_path)
    if "无差异" in diff_result:
        return True
    else:
        # 打印测试样例编号和差异信息
        test_case_name = os.path.basename(test_dir)
        error_msg = f"测试失败：{test_case_name}"
        print(error_msg)
        result_file.write(error_msg + "\n")
        print(diff_result)
        result_file.write(diff_result + "\n")
        return False

def main():
    # 设置路径
    test_root = "test"  # 测试根目录
    source_files = ["Richc.c", "cJSON.c"]  # C源文件列表
    executable = "./richc"   # 可执行文件名称

    # 打开结果文件
    with open("result.txt", "w", encoding="utf-8") as result_file:
        # 写入测试开始时间
        start_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        header = f"测试开始时间: {start_time}\n"
        header += "=" * 50 + "\n"
        print(header)
        result_file.write(header)
        
        # 编译C程序 - 添加UTF-8编译选项
        if not os.path.exists(executable):
            print("编译C程序...")
            result_file.write("编译C程序...\n")
            compile_command = ['gcc', '-o', executable] + source_files + ['-lm', '-fexec-charset=UTF-8']
            compile_result = subprocess.run(compile_command)
            if compile_result.returncode != 0:
                error_msg = "极可能缺少cJSON.c文件，编译失败"
                print(error_msg)
                result_file.write(error_msg + "\n")
                return

        # 获取所有测试样例目录
        test_cases = []
        if os.path.exists(test_root):
            for name in os.listdir(test_root):
                path = os.path.join(test_root, name)
                if os.path.isdir(path):
                    test_cases.append(path)
        test_cases.sort()  # 按目录名排序

        if not test_cases:
            error_msg = "未找到测试样例"
            print(error_msg)
            result_file.write(error_msg + "\n")
            return

        # 设置控制台编码为UTF-8（仅Windows）
        if sys.platform == "win32":
            os.system("chcp 65001 > nul")  # 设置控制台代码页为UTF-8

        # 运行每个测试样例
        passed = 0
        total = len(test_cases)
        for test_dir in test_cases:
            test_case_name = os.path.basename(test_dir)
            test_info = f"测试样例：{test_case_name}"
            print(test_info)
            result_file.write(test_info + "\n")
            if run_test(test_dir, executable, result_file):
                success_msg = "通过"
                print(success_msg)
                result_file.write(success_msg + "\n")
                passed += 1
            else:
                fail_msg = "失败"
                print(fail_msg)
                result_file.write(fail_msg + "\n")
            print()  # 空行分隔
            result_file.write("\n")

        # 写入测试结果总结
        end_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        footer = "=" * 50 + "\n"
        footer += f"测试结束时间: {end_time}\n"
        footer += f"测试完成：通过 {passed}/{total}\n"
        print(footer)
        result_file.write(footer)

if __name__ == '__main__':
    main()