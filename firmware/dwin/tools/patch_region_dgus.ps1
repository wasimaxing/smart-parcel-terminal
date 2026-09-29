param(
    [Parameter(Mandatory = $true)]
    [string]$GeneratedPng,
    [string]$ProjectRoot = ''
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($ProjectRoot)) {
    $ProjectRoot = Split-Path -Parent $PSScriptRoot
}
$setDir = Join-Path $ProjectRoot 'DWIN_SET'
$assetDir = Join-Path $ProjectRoot 'assets'
New-Item -ItemType Directory -Force -Path $assetDir | Out-Null

# Preserve the selected ImageGen result in the project and convert it to the
# exact 800x480, 24-bit BMP format used by the DGUS screen.
$sourceCopy = Join-Path $assetDir '01_balanced_tier_cards_imagegen.png'
$sourceResolved = (Resolve-Path -LiteralPath $GeneratedPng).Path
$copyResolved = if (Test-Path -LiteralPath $sourceCopy) {
    (Resolve-Path -LiteralPath $sourceCopy).Path
}
else {
    ''
}
if (-not $sourceResolved.Equals($copyResolved,
        [StringComparison]::OrdinalIgnoreCase)) {
    Copy-Item -LiteralPath $GeneratedPng -Destination $sourceCopy -Force
}
$background = Join-Path $setDir '01.bmp'
$backgroundBackup = Join-Path $assetDir '01_before_region.bmp'
if (-not (Test-Path -LiteralPath $backgroundBackup)) {
    Copy-Item -LiteralPath $background -Destination $backgroundBackup
}
Add-Type -AssemblyName System.Drawing
$regionJpeg = $null
$inputImage = [System.Drawing.Image]::FromFile($sourceCopy)
try {
    $outputImage = New-Object System.Drawing.Bitmap 800, 480,
        ([System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    try {
        $graphics = [System.Drawing.Graphics]::FromImage($outputImage)
        try {
            $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $graphics.DrawImage($inputImage, 0, 0, 800, 480)
        }
        finally {
            $graphics.Dispose()
        }
        # GDI+ cannot reliably overwrite an existing bitmap in place.  Save
        # once to a temporary file, then use normal file replacement so the
        # script remains safe to run after every DGUS regeneration.
        $temporaryBmp = Join-Path $assetDir '01_region.building.bmp'
        $outputImage.Save($temporaryBmp, [System.Drawing.Imaging.ImageFormat]::Bmp)
        $regionBmp = Join-Path $assetDir '01_balanced_tier_cards.bmp'
        $temporaryHash = (Get-FileHash -LiteralPath $temporaryBmp -Algorithm SHA256).Hash
        if (-not (Test-Path -LiteralPath $background) -or
            (Get-FileHash -LiteralPath $background -Algorithm SHA256).Hash -ne $temporaryHash) {
            Copy-Item -LiteralPath $temporaryBmp -Destination $background -Force
        }
        if (-not (Test-Path -LiteralPath $regionBmp) -or
            (Get-FileHash -LiteralPath $regionBmp -Algorithm SHA256).Hash -ne $temporaryHash) {
            Copy-Item -LiteralPath $temporaryBmp -Destination $regionBmp -Force
        }
        Remove-Item -LiteralPath $temporaryBmp

        # DGUS background libraries store baseline JPEG data.  Encode the new
        # page at the same quality family as the files produced by DGUS.
        $jpegCodec = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() |
            Where-Object { $_.MimeType -eq 'image/jpeg' } |
            Select-Object -First 1
        $jpegParams = New-Object System.Drawing.Imaging.EncoderParameters 1
        $jpegParams.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter(
            [System.Drawing.Imaging.Encoder]::Quality, [long]90)
        $jpegStream = New-Object IO.MemoryStream
        try {
            $outputImage.Save($jpegStream, $jpegCodec, $jpegParams)
            $regionJpeg = $jpegStream.ToArray()
        }
        finally {
            $jpegStream.Dispose()
            $jpegParams.Dispose()
        }
    }
    finally {
        $outputImage.Dispose()
    }
}
finally {
    $inputImage.Dispose()
}

function Read-Be16 {
    param([byte[]]$Bytes, [int]$Offset)
    return (($Bytes[$Offset] -shl 8) -bor $Bytes[$Offset + 1])
}

function Read-Le16 {
    param([byte[]]$Bytes, [int]$Offset)
    return ($Bytes[$Offset] -bor ($Bytes[$Offset + 1] -shl 8))
}

function Read-Be32 {
    param([byte[]]$Bytes, [int]$Offset)
    return [uint32](
        ([uint32]$Bytes[$Offset] -shl 24) -bor
        ([uint32]$Bytes[$Offset + 1] -shl 16) -bor
        ([uint32]$Bytes[$Offset + 2] -shl 8) -bor
        [uint32]$Bytes[$Offset + 3])
}

function Write-Be16 {
    param([IO.Stream]$Stream, [uint32]$Value)
    $Stream.WriteByte(($Value -shr 8) -band 0xFF)
    $Stream.WriteByte($Value -band 0xFF)
}

function Write-Be32 {
    param([IO.Stream]$Stream, [uint32]$Value)
    $Stream.WriteByte(($Value -shr 24) -band 0xFF)
    $Stream.WriteByte(($Value -shr 16) -band 0xFF)
    $Stream.WriteByte(($Value -shr 8) -band 0xFF)
    $Stream.WriteByte($Value -band 0xFF)
}

function Get-IclJpeg {
    param([byte[]]$Bytes, [int]$ChunkStart, [int]$ChunkEnd)
    $jpegStart = -1
    $jpegEnd = -1
    for ($i = $ChunkStart; $i -lt $ChunkEnd - 1; $i++) {
        if ($Bytes[$i] -eq 0xFF -and $Bytes[$i + 1] -eq 0xD8) {
            $jpegStart = $i
            break
        }
    }
    for ($i = $ChunkEnd - 2; $i -ge $jpegStart; $i--) {
        if ($Bytes[$i] -eq 0xFF -and $Bytes[$i + 1] -eq 0xD9) {
            $jpegEnd = $i + 1
            break
        }
    }
    if ($jpegStart -lt 0 -or $jpegEnd -lt $jpegStart) {
        throw 'Invalid JPEG payload in DGUS ICL file.'
    }
    return [byte[]]$Bytes[$jpegStart..$jpegEnd]
}

function New-IclChunk {
    param([byte[]]$Jpeg, [int]$Width = 800, [int]$Height = 480)
    $padding = 8
    while (((10 + $Jpeg.Length + $padding) % 4) -ne 0) {
        $padding++
    }
    $chunkLength = 10 + $Jpeg.Length + $padding
    if ($chunkLength -le 0x27C) {
        throw 'JPEG payload is too short for the DGUS ICL chunk format.'
    }
    $stream = New-Object IO.MemoryStream
    Write-Be16 $stream $Width
    Write-Be16 $stream $Height
    Write-Be16 $stream 0x0272
    Write-Be32 $stream ($chunkLength - 0x27C)
    $stream.Write($Jpeg, 0, $Jpeg.Length)
    for ($i = 0; $i -lt $padding; $i++) {
        $stream.WriteByte(0)
    }
    $result = $stream.ToArray()
    $stream.Dispose()
    return [byte[]]$result
}

function Get-Crc16Modbus {
    param([byte[]]$Bytes, [int]$Start)
    [uint16]$crc = 0xFFFF
    for ($i = $Start; $i -lt $Bytes.Length; $i++) {
        $crc = $crc -bxor $Bytes[$i]
        for ($bit = 0; $bit -lt 8; $bit++) {
            if (($crc -band 1) -ne 0) {
                $crc = [uint16](($crc -shr 1) -bxor 0xA001)
            }
            else {
                $crc = [uint16]($crc -shr 1)
            }
        }
    }
    return $crc
}

# Rebuild 32bg.icl while preserving pages 00 and 02 byte-for-byte at the
# JPEG level.  This removes the need for a manual click in the DGUS GUI and
# keeps the generated library compatible with its documented container form.
$iclPath = Join-Path $setDir '32bg.icl'
$iclBackup = Join-Path $assetDir '32bg_before_region.icl'
if (-not (Test-Path -LiteralPath $iclBackup)) {
    Copy-Item -LiteralPath $iclPath -Destination $iclBackup
}
$icl = [IO.File]::ReadAllBytes($iclBackup)
if ($icl.Length -lt 32 -or
    [Text.Encoding]::ASCII.GetString($icl, 0, 6) -ne 'DGUS_3') {
    throw 'Unexpected 32bg.icl signature; refusing to rebuild.'
}
$imageCount = (Read-Le16 $icl 14) + 1
if ($imageCount -ne 3) {
    throw "Expected 3 DGUS backgrounds, found $imageCount."
}
$headerLength = 16 + (4 * $imageCount)
$chunkStarts = @($headerLength)
for ($i = 1; $i -lt $imageCount; $i++) {
    $chunkStarts += [int](Read-Be32 $icl (16 + 4 * $i))
}
$jpegs = @()
for ($i = 0; $i -lt $imageCount; $i++) {
    $chunkEnd = if ($i + 1 -lt $imageCount) {
        $chunkStarts[$i + 1]
    }
    else {
        $icl.Length
    }
    $jpegs += ,(Get-IclJpeg $icl $chunkStarts[$i] $chunkEnd)
}
$jpegs[1] = $regionJpeg
$chunks = @()
foreach ($jpeg in $jpegs) {
    $chunks += ,(New-IclChunk $jpeg)
}
$newIcl = New-Object IO.MemoryStream
$signature = [Text.Encoding]::ASCII.GetBytes('DGUS_3')
$newIcl.Write($signature, 0, $signature.Length)
$newIcl.WriteByte(0); $newIcl.WriteByte(0) # CRC placeholder
$newIcl.Write((New-Object byte[] 4), 0, 4) # length placeholder
$newIcl.WriteByte(0x04); $newIcl.WriteByte(0x00)
$newIcl.WriteByte(($imageCount - 1) -band 0xFF)
$newIcl.WriteByte((($imageCount - 1) -shr 8) -band 0xFF)
Write-Be32 $newIcl 0
$nextOffset = $headerLength + $chunks[0].Length
for ($i = 1; $i -lt $imageCount; $i++) {
    Write-Be32 $newIcl $nextOffset
    $nextOffset += $chunks[$i].Length
}
foreach ($chunk in $chunks) {
    $newIcl.Write($chunk, 0, $chunk.Length)
}
$newIclBytes = $newIcl.ToArray()
$newIcl.Dispose()
$payloadLength = $newIclBytes.Length - 8
$newIclBytes[8] = ($payloadLength -shr 24) -band 0xFF
$newIclBytes[9] = ($payloadLength -shr 16) -band 0xFF
$newIclBytes[10] = ($payloadLength -shr 8) -band 0xFF
$newIclBytes[11] = $payloadLength -band 0xFF
$crc = Get-Crc16Modbus $newIclBytes 8
$newIclBytes[6] = $crc -band 0xFF
$newIclBytes[7] = ($crc -shr 8) -band 0xFF
[IO.File]::WriteAllBytes($iclPath, $newIclBytes)

# Bind the two page-0 text controls to dedicated fixed-length ASCII buffers.
# The numeric system state remains at VP 0x1031; writing that U16 directly to
# a text control would produce control characters rather than readable text.
$showPath = Join-Path $setDir '14ShowFile.bin'
$showBackup = Join-Path $assetDir '14ShowFile_before_region.bin'
if (-not (Test-Path -LiteralPath $showBackup)) {
    Copy-Item -LiteralPath $showPath -Destination $showBackup
}
$show = [IO.File]::ReadAllBytes($showPath)
if ($show.Length -lt 0x4080 -or $show[0x4040] -ne 0x5A -or
    $show[0x4060] -ne 0x5A) {
    throw 'Unexpected 14ShowFile.bin layout; refusing to patch.'
}
$show[0x4046] = 0x12
$show[0x4047] = 0x20  # state text VP 0x1220
$show[0x4066] = 0x12
$show[0x4067] = 0x30  # status text VP 0x1230
$show[0x4077] = 0x14  # status text length: 20 bytes

# The four page-1 parameter displays occupied the final four 32-byte records
# at 0x40C0..0x413F.  The fixed-choice page no longer shows parameter boxes,
# so terminate the display-control table before these records.
for ($i = 0x40C0; $i -lt 0x4140; ++$i) {
    $show[$i] = 0xFF
}
[IO.File]::WriteAllBytes($showPath, $show)

function New-ReturnKeyRecord {
    param([int]$X1, [int]$Y1, [int]$X2, [int]$Y2, [int]$Action)
    $record = New-Object byte[] 32
    $record[0] = 0x00; $record[1] = 0x01 # page 1
    $record[2] = (($X1 -shr 8) -band 0xFF); $record[3] = ($X1 -band 0xFF)
    $record[4] = (($Y1 -shr 8) -band 0xFF); $record[5] = ($Y1 -band 0xFF)
    $record[6] = (($X2 -shr 8) -band 0xFF); $record[7] = ($X2 -band 0xFF)
    $record[8] = (($Y2 -shr 8) -band 0xFF); $record[9] = ($Y2 -band 0xFF)
    $record[10] = 0xFF; $record[11] = 0x00
    $record[12] = 0xFF; $record[13] = 0x00
    $record[14] = 0xFD; $record[15] = 0x05
    $record[16] = 0xFE; $record[17] = 0x11
    $record[18] = 0x00; $record[19] = 0x00 # VP 0x1100
    $record[20] = 0x00; $record[21] = $Action
    return $record
}

function New-PageSwitchRecord {
    param(
        [int]$Page, [int]$X1, [int]$Y1, [int]$X2, [int]$Y2,
        [int]$TargetPage
    )
    $record = New-Object byte[] 16
    $record[0] = (($Page -shr 8) -band 0xFF); $record[1] = ($Page -band 0xFF)
    $record[2] = (($X1 -shr 8) -band 0xFF); $record[3] = ($X1 -band 0xFF)
    $record[4] = (($Y1 -shr 8) -band 0xFF); $record[5] = ($Y1 -band 0xFF)
    $record[6] = (($X2 -shr 8) -band 0xFF); $record[7] = ($X2 -band 0xFF)
    $record[8] = (($Y2 -shr 8) -band 0xFF); $record[9] = ($Y2 -band 0xFF)
    $record[10] = (($TargetPage -shr 8) -band 0xFF)
    $record[11] = ($TargetPage -band 0xFF)
    $record[12] = 0xFF; $record[13] = 0x00
    return $record
}

# Page 1 is fixed-tier selection only.  Deliberately omit all four data-entry
# touch controls and the former save action from 13TouchFile.bin.
$touchPath = Join-Path $setDir '13TouchFile.bin'
$touchBackup = Join-Path $assetDir '13TouchFile_before_region.bin'
if (-not (Test-Path -LiteralPath $touchBackup)) {
    Copy-Item -LiteralPath $touchPath -Destination $touchBackup
}
$touch = [IO.File]::ReadAllBytes($touchBackup)
if ($touch.Length -lt 2 -or $touch[$touch.Length - 2] -ne 0xFF -or
    $touch[$touch.Length - 1] -ne 0xFF) {
    throw 'Unexpected 13TouchFile.bin terminator; refusing to patch.'
}
$page0Length = 0x70
if ($touch.Length -lt ($page0Length + 2)) {
    throw 'Unexpected 13TouchFile.bin layout; page-0 controls are incomplete.'
}
$records = @(
    (New-ReturnKeyRecord 32 112 262 352 6),
    (New-ReturnKeyRecord 285 112 512 352 7),
    (New-ReturnKeyRecord 535 112 764 352 8)
)
$output = New-Object IO.MemoryStream
$output.Write($touch, 0, $page0Length)
$backRecord = New-PageSwitchRecord 1 28 394 772 464 0
$output.Write($backRecord, 0, $backRecord.Length)
foreach ($record in $records) {
    $output.Write($record, 0, $record.Length)
}
$output.WriteByte(0xFF)
$output.WriteByte(0xFF)
[IO.File]::WriteAllBytes($touchPath, $output.ToArray())
$output.Dispose()

Write-Output "Patched $background"
Write-Output "Rebuilt $iclPath ($($newIclBytes.Length) bytes, CRC 0x$($crc.ToString('X4')))"
Write-Output "Patched $showPath"
Write-Output "Patched $touchPath"
