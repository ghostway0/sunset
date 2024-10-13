import argparse
from typing import List, Union
from enum import StrEnum
from dataclasses import dataclass


class Backend(StrEnum):
    VULKAN = "vulkan"
    OPENGL = "opengl"


class BaseType(StrEnum):
    F32 = "f32"
    F64 = "f64"
    I32 = "i32"
    I64 = "i64"
    BYTE = "byte"

    def c_str(self):
        if self == BaseType.F32:
            return "float {}"
        elif self == BaseType.F64:
            return "double {}"
        elif self == BaseType.I32:
            return "int {}"
        elif self == BaseType.I64:
            return "long {}"
        elif self == BaseType.BYTE:
            return "char {}"

    def size(self):
        if self == BaseType.F32:
            return 4
        elif self == BaseType.F64:
            return 8
        elif self == BaseType.I32:
            return 4
        elif self == BaseType.I64:
            return 8
        elif self == BaseType.BYTE:
            return 1

    def size_aligned(self):
        return self.size()

    def alignment(self):
        return self.size()


@dataclass
class Type:
    base: Union["Type", BaseType]
    count: int = 1

    def __str__(self):
        return f"{self.base}{'[' + str(self.count) + ']' if self.count > 1 else ''}"

    def c_str(self):
        if self.count == 1:
            return self.base.c_str()

        return self.base.c_str().format(
            f"{{}}[{self.count + self.alignment_padding()}]"
        )

    def size(self):
        return self.base.size() * self.count

    def size_aligned(self):
        return self.base.size() * (self.count + self.alignment_padding())

    def alignment_padding(self):
        return (
            (-self.size()) % self.alignment() // self.base.size_aligned()
            if self.count > 1
            else 0
        )

    def alignment(self):
        if self.count % 3 == 0:
            return 16

        return self.base.alignment() if self.count == 1 else 16


def parse_type(ty: str):
    parts = ty.split("[")

    if len(parts) == 1:
        return Type(BaseType(parts[0]))

    dimensions = [int(part.strip("]")) for part in parts[1:]]
    ty = BaseType(parts[0])

    for dim in reversed(dimensions):
        ty = Type(ty, dim)

    return ty


class Signature:
    def __init__(self, name: str, file: str, signature: str):
        def parse_signature(signature: str):
            args = signature.split(",")

            for arg in args:
                parts = arg.split(":")

                if len(parts) != 2:
                    raise ValueError(f"Invalid signature {signature}")

                name, ty = parts
                yield name.strip(), parse_type(ty.strip())

        self.name = name
        self.file = file
        self.signature = list(parse_signature(signature))

    def __str__(self):
        return (
            f"{self.name}({', '.join(f'{name}: {ty}' for name, ty in self.signature)})"
        )


def parse_content(content, output):
    signatures = []

    for line in content.split("\n"):
        if not line:
            continue

        parts = line.split("]", 1)

        if len(parts) != 2:
            raise ValueError(f"Invalid line {line}")

        file, rest = parts
        name, signature = rest.split("(")

        signatures.append(Signature(name, file, signature[:-1]))

    return signatures


def write_header(f, signatures: List[Signature], _: Backend):
    f.write("#pragma once\n\n")

    for signature in signatures:
        f.write(f"struct {signature.name}_uniform {{\n")
        curr_size = 0

        for name, ty in signature.signature:
            required_alignment = ty.alignment()

            if curr_size % required_alignment != 0:
                padding = required_alignment - (curr_size % required_alignment)
                f.write(f"    char _padding{curr_size}[{padding}];\n")

            f.write(f"    {ty.c_str().format(name)};\n")

            curr_size += ty.size()

        f.write("} __attribute__((packed));\n\n")

        f.write(
            f'static const char *{signature.name}_signature = "{
                str(signature)}";\n'
        )


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=str)
    parser.add_argument("--output", "-o", type=str, required=True)
    parser.add_argument("--backend", "-b", type=Backend, required=True)
    args = parser.parse_args()

    with open(args.input, "r") as f:
        content = f.read()

    signatures = parse_content(content, args.output)

    with open(args.output, "w") as f:
        write_header(f, signatures, args.backend)
