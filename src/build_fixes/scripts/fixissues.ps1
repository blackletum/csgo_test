$filePaths = @(
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\code_generator.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\command_line_interface.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\cpp\cpp_enum_field.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\cpp\cpp_enum.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\cpp\cpp_extension.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\cpp\cpp_field.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\cpp\cpp_file.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\cpp\cpp_generator.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\cpp\cpp_helpers.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\cpp\cpp_map_field.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\cpp\cpp_message_field.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\cpp\cpp_message.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\cpp\cpp_padding_optimizer.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\cpp\cpp_primitive_field.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\cpp\cpp_service.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\cpp\cpp_string_field.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\plugin.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\plugin.pb.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\python\python_generator.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\subprocess.cc",
    "D:\Development\Linux-Strike-original\thirdparty\protobuf-3.5.1\src\google\protobuf\compiler\zip_writer.cc"
)

$lineToInject = '#include "intrin_workarounds.h"'

foreach ($filePath in $filePaths) {
    if (Test-Path $filePath) {
        $lines = Get-Content $filePath
        $insertIndex = 0

        for ($i = 0; $i -lt $lines.Count; $i++) {
            $line = $lines[$i].Trim()

            if ($line -ne "" -and -not ($line.StartsWith("//") -or $line.StartsWith("/*") -or $line.StartsWith("*"))) {
                $insertIndex = $i
                break
            }
        }

        if ($lines -notcontains $lineToInject) {
            $newLines = $lines[0..($insertIndex - 1)] + $lineToInject + $lines[$insertIndex..($lines.Count - 1)]
            $newLines | Out-File -FilePath $filePath -Encoding utf8
            Write-Host "Injected into: $filePath"
        } else {
            Write-Host "Already contains line: $filePath"
        }
    } else {
        Write-Host "File not found: $filePath"
    }
}
