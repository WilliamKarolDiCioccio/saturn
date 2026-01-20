"""
JNI code generator for Kotlin @NativeExport functions.
Generates C++ code to load and call Kotlin functions from native code.
Supports multiple classes with automatic class detection.
"""

import re
import os
import argparse
from pathlib import Path
from typing import List, Tuple, Optional, Dict
from collections import defaultdict

def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Generate JNI loading code for Kotlin @NativeExport functions"
    )
    
    parser.add_argument(
        "-i", "--input",
        type=str,
        nargs="+",
        help="Input Kotlin files or directories to scan (supports wildcards)"
    )
    
    parser.add_argument(
        "-o", "--output",
        type=str,
        default="jni_loader.cpp",
        help="Output C++ file path (default: jni_loader.cpp)"
    )
    
    parser.add_argument(
        "--helper-class",
        type=str,
        default="saturn::platform::agdk::JNIHelper",
        help="JNI helper class name (default: saturn::platform::agdk::JNIHelper)"
    )
    
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Enable verbose output"
    )
    
    return parser.parse_args()

def kotlin_to_jni_type(ktype: str, nullable: bool = False) -> str:
    """
    Convert Kotlin type to JNI signature.
    
    Args:
        ktype: Kotlin type name (e.g., "Int", "String")
        nullable: Whether the type is nullable
    
    Returns:
        JNI type signature or None if unknown
    """
    # Handle array types
    if ktype.endswith("Array"):
        element_type = ktype[:-5]  # Remove "Array" suffix
        element_sig = kotlin_to_jni_type(element_type, False)
        if element_sig:
            return "[" + element_sig
        return None
    
    # Basic type mappings
    mapping = {
        "Int": "I",
        "Float": "F", 
        "Double": "D",
        "Long": "J",
        "Boolean": "Z",
        "Byte": "B",
        "Short": "S",
        "Char": "C",
        "String": "Ljava/lang/String;",
        "Unit": "V"
    }
    
    base_sig = mapping.get(ktype)
    if not base_sig:
        return None
    
    # For nullable primitive types, we need to use wrapper objects
    if nullable and base_sig in "IFDJZBSC":
        wrapper_mapping = {
            "I": "Ljava/lang/Integer;",
            "F": "Ljava/lang/Float;",
            "D": "Ljava/lang/Double;", 
            "J": "Ljava/lang/Long;",
            "Z": "Ljava/lang/Boolean;",
            "B": "Ljava/lang/Byte;",
            "S": "Ljava/lang/Short;",
            "C": "Ljava/lang/Character;"
        }
        return wrapper_mapping[base_sig]
    
    return base_sig

def parse_kotlin_type(type_str: str) -> Tuple[str, bool]:
    """
    Parse a Kotlin type string to extract type name and nullability.
    
    Args:
        type_str: Kotlin type string (e.g., "Int?", "String", "IntArray?")
    
    Returns:
        Tuple of (type_name, is_nullable)
    """
    type_str = type_str.strip()
    is_nullable = type_str.endswith("?")
    
    if is_nullable:
        type_str = type_str[:-1]  # Remove '?' suffix
    
    return type_str, is_nullable

def extract_package_and_class(file_content: str, file_path: Path) -> Optional[str]:
    """
    Extract the full class name (package + class) from Kotlin file content.
    
    Args:
        file_content: Content of the Kotlin file
        file_path: Path to the Kotlin file
    
    Returns:
        Full JNI class name (e.g. "com/example/MyClass") or None if not found
    """
    # Extract package declaration
    package_match = re.search(r'package\s+([a-zA-Z0-9_.]+)', file_content)
    package_name = package_match.group(1) if package_match else None
    
    # Extract class/object name - improved regex to avoid matching annotations
    class_patterns = [
        # Match class/object declarations, avoiding annotations
        r'(?:^|\n)\s*(?:(?:public|private|internal|protected)\s+)?(?:class|object)\s+(\w+)',
        # Match class/object with modifiers but not preceded by @
        r'(?:^|\n)\s*(?:(?:abstract|final|open|sealed|data|inner)\s+)*(?:class|object)\s+(\w+)',
        # Fallback: simple class/object match not preceded by @
        r'(?<!@\w{0,20})\b(?:class|object)\s+(\w+)'
    ]
    
    class_name = None
    for pattern in class_patterns:
        class_matches = re.finditer(pattern, file_content, re.MULTILINE)
        for match in class_matches:
            candidate = match.group(1)
            # Skip if this looks like an annotation (starts with capital but is likely annotation name)
            if candidate != "NativeExport" and not candidate.startswith("Native"):
                class_name = candidate
                break
        if class_name:
            break
    
    # If no explicit class found, try to infer from filename
    if not class_name:
        class_name = file_path.stem
        # Convert from PascalCase/camelCase filename to class name
        if not class_name[0].isupper():
            class_name = class_name[0].upper() + class_name[1:]
    
    # Construct full class name
    if package_name and class_name:
        return package_name.replace('.', '/') + '/' + class_name
    elif class_name:
        # No package, just class name
        return class_name
    
    return None

def find_kotlin_files(input_paths: List[str]) -> List[Path]:
    """
    Find all Kotlin files from input paths.
    
    Args:
        input_paths: List of file paths, directory paths, or glob patterns
    
    Returns:
        List of Path objects for .kt files
    """
    kotlin_files = []
    
    for input_path in input_paths:
        path = Path(input_path)
        
        if path.is_file() and path.suffix == ".kt":
            kotlin_files.append(path)
        elif path.is_dir():
            kotlin_files.extend(path.rglob("*.kt"))
        else:
            # Try as glob pattern
            kotlin_files.extend(Path(".").glob(input_path))
    
    return list(set(kotlin_files))  # Remove duplicates

def extract_native_exports(kotlin_files: List[Path], verbose: bool = False) -> Dict[str, List[Tuple[str, str]]]:
    """
    Extract @NativeExport functions from Kotlin files, grouped by class.
    
    Args:
        kotlin_files: List of Kotlin files to scan
        verbose: Enable verbose output
    
    Returns:
        Dictionary mapping class names to lists of (function_name, jni_signature) tuples
    """
    # Enhanced regex to handle @JvmStatic and @NativeExport annotations
    method_pattern = re.compile(
        r"(?:@JvmStatic\s*)?@NativeExport\s*fun\s+(?P<name>\w+)\s*\((?P<params>[^)]*)\)\s*(?::\s*(?P<ret>[^{\s]+))?",
        re.MULTILINE
    )
    
    class_methods = defaultdict(list)
    
    for kt_file in kotlin_files:
        if verbose:
            print(f"Scanning: {kt_file}")
        
        try:
            text = kt_file.read_text(encoding='utf-8')
        except Exception as e:
            print(f"Warning: Could not read {kt_file}: {e}")
            continue
        
        # Extract class information
        class_name = extract_package_and_class(text, kt_file)
        if not class_name:
            print(f"Warning: Could not determine class name for {kt_file}")
            continue
        
        if verbose:
            print(f"  Class: {class_name}")
        
        # Find all @NativeExport methods in this file
        methods_found = []
        for match in method_pattern.finditer(text):
            name = match.group("name")
            params = match.group("params").strip()
            ret = match.group("ret") or "Unit"
            
            if verbose:
                print(f"  Found function: {name}")
            
            # Parse parameter types
            sig_parts = []
            param_parsing_success = True
            
            if params:
                for param in params.split(','):
                    param = param.strip()
                    if ':' not in param:
                        print(f"Warning: Invalid parameter format in {name}: {param}")
                        param_parsing_success = False
                        break
                    
                    # Split on last ':' to handle cases like "param: List<String>"
                    colon_idx = param.rfind(':')
                    param_type = param[colon_idx + 1:].strip()
                    
                    type_name, is_nullable = parse_kotlin_type(param_type)
                    sig = kotlin_to_jni_type(type_name, is_nullable)
                    
                    if not sig:
                        print(f"Error: Unknown parameter type in {name}: {param_type}")
                        param_parsing_success = False
                        break
                    
                    sig_parts.append(sig)
            
            # Only proceed if parameter parsing was successful (or no parameters)
            if param_parsing_success:
                # Parse return type
                ret_type, ret_nullable = parse_kotlin_type(ret)
                ret_sig = kotlin_to_jni_type(ret_type, ret_nullable)
                
                if not ret_sig:
                    print(f"Error: Unknown return type in {name}: {ret}")
                    continue
                
                signature = "(" + "".join(sig_parts) + ")" + ret_sig
                methods_found.append((name, signature))
                
                if verbose:
                    print(f"    Signature: {signature}")
        
        # Add methods to the class
        if methods_found:
            class_methods[class_name].extend(methods_found)
    
    return dict(class_methods)

def generate_cpp_file(class_methods: Dict[str, List[Tuple[str, str]]], output_path: Path, helper_class: str):
    """
    Generate the C++ JNI loader file.
    
    Args:
        class_methods: Dictionary mapping class names to method lists
        output_path: Output file path
        helper_class: JNI helper class name
    """
    total_methods = sum(len(methods) for methods in class_methods.values())
    
    with output_path.open('w', encoding='utf-8') as f:
        f.write("// Auto-generated JNI loader code\n")
        f.write("// Generated by jni_generator.py\n")
        f.write(f"// Found {len(class_methods)} classes with {total_methods} methods\n\n")
        
        f.write("JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)\n")
        f.write("{\n")
        f.write("    saturn::tools::Logger::initialize();\n\n")
        
        f.write(f"    auto helper = {helper_class}::getInstance();\n\n")
        
        f.write("    helper->initialize(vm);\n\n")
        
        # Process each class
        for class_name, methods in class_methods.items():
            f.write(f"    // Load methods for {class_name}\n")
            f.write(f"    {{\n")
            f.write(f"        auto clazzRef = helper->findClass(\"{class_name}\");\n")
            f.write(f"        if (!clazzRef)\n")
            f.write(f"        {{\n")
            f.write(f"            SATURN_ERROR(\"Failed to find {class_name.split('/')[-1]} class\");\n")
            f.write(f"            return JNI_ERR;\n")
            f.write(f"        }}\n\n")
            
            f.write(f"        helper->createGlobalRef(clazzRef);\n\n")
            
            # Generate getStaticMethodID calls for each method in this class
            for name, sig in methods:
                f.write(f"        helper->getStaticMethodID(\"{class_name}\", \"{name}\",\n")
                f.write(f"                                  \"{sig}\");\n\n")
            
            f.write(f"    }}\n\n")
        
        f.write("    return JNI_VERSION_1_6;\n")
        f.write("}\n")
        
        f.write("\nJNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* /*reserved*/)\n")
        f.write("{\n")
        f.write("    saturn::platform::agdk::JNIHelper::getInstance()->shutdown();\n\n")
        f.write("    saturn::tools::Logger::shutdown();\n")
        f.write("}\n")

def main():
    """Main entry point."""
    args = parse_arguments()
    
    # Default input if none provided
    if not args.input:
        args.input = ["engine_bridge/src/main/java"]
    
    # Find Kotlin files
    kotlin_files = find_kotlin_files(args.input)
    
    if not kotlin_files:
        print("Error: No Kotlin files found in specified input paths")
        return 1
    
    if args.verbose:
        print(f"Found {len(kotlin_files)} Kotlin files")
    
    # Extract native exports grouped by class
    class_methods = extract_native_exports(kotlin_files, args.verbose)
    
    if not class_methods:
        print("Warning: No @NativeExport methods found")
        return 0
    
    # Generate output file
    output_path = Path(args.output)
    generate_cpp_file(class_methods, output_path, args.helper_class)
    
    total_methods = sum(len(methods) for methods in class_methods.values())
    print(f"Generated {output_path} with {len(class_methods)} classes and {total_methods} method loaders.")
    
    if args.verbose:
        print("\nClasses and methods:")
        for class_name, methods in class_methods.items():
            print(f"  {class_name}:")
            for name, sig in methods:
                print(f"    {name}: {sig}")
    
    return 0

if __name__ == "__main__":
    exit(main())