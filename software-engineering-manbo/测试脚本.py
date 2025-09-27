      
import os
import subprocess
import json
from collections import defaultdict

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
    
    # 如果是财神位置字段，跳过比较
    if path.endswith(".god.location"):
        return differences  # 跳过财神位置的比较
    
    if isinstance(expect_data, dict):
        # 对于字典，只检查expect中存在的键
        for key in expect_data:
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
        with open(dump_path, 'r') as dump_file:
            dump_data = json.load(dump_file)
        with open(expect_path, 'r') as expect_file:
            expect_data = json.load(expect_file)
    except json.JSONDecodeError as e:
        return f"JSON解析错误: {e}"
    
    differences = compare_json_with_rules(dump_data, expect_data)
    
    if differences:
        return "发现差异:\n" + "\n".join(differences)
    else:
        return "无差异"

def run_test(test_dir, executable):
    """
    运行单个测试样例
    :param test_dir: 测试目录路径
    :param executable: 可执行文件路径
    :return: 测试是否通过（布尔值）
    """
    # 检查测试目录是否存在
    if not os.path.isdir(test_dir):
        print(f"测试目录不存在: {test_dir}")
        return False

    test_txt_path = os.path.join(test_dir, "test.txt")
    expect_path = os.path.join(test_dir, "expect.json")
    dump_path = os.path.join(test_dir, "dump.json")

    # 检查必要的文件是否存在
    if not os.path.exists(test_txt_path):
        print(f"缺少test.txt文件: {test_txt_path}")
        return False
    if not os.path.exists(expect_path):
        print(f"缺少expect.json文件: {expect_path}")
        return False

    # 读取测试命令
    with open(test_txt_path, 'r') as f:
        input_commands = f.read()

    # 运行程序，将测试目录作为参数传递，并将test.txt内容作为标准输入
    process = subprocess.Popen([executable, test_dir], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate(input=input_commands.encode())

    # 检查程序是否成功运行
    if process.returncode != 0:
        print(f"程序运行失败，返回码：{process.returncode}")
        print("标准错误输出：")
        print(stderr.decode())
        return False

    # 检查dump.json是否生成
    if not os.path.exists(dump_path):
        print("dump.json未生成")
        return False

    # 比较dump.json和expect.json的内容
    diff_result = compare_json_files(dump_path, expect_path)
    if "无差异" in diff_result:
        return True
    else:
        # 打印测试样例编号和差异信息
        test_case_name = os.path.basename(test_dir)
        print(f"测试失败：{test_case_name}")
        print(diff_result)
        return False

def main():
    # 设置路径
    test_root = "test"  # 测试根目录
    source_files = ["Richc.c", "cJSON.c"]  # C源文件列表
    executable = "./richc"   # 可执行文件名称

    # 编译C程序
    if not os.path.exists(executable):
        print("编译C程序...")
        compile_command = ['gcc', '-o', executable] + source_files + ['-lm']
        compile_result = subprocess.run(compile_command)
        if compile_result.returncode != 0:
            print("极可能缺少cJSON.c文件，编译失败")
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
        print("未找到测试样例")
        return

    # 运行每个测试样例
    passed = 0
    total = len(test_cases)
    for test_dir in test_cases:
        test_case_name = os.path.basename(test_dir)
        print(f"测试样例：{test_case_name}")
        if run_test(test_dir, executable):
            print("通过")
            passed += 1
        else:
            print("失败")
        print()  # 空行分隔

    print(f"测试完成：通过 {passed}/{total}")

if __name__ == '__main__':
    main()

    