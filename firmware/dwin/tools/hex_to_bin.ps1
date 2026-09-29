param(
    [Parameter(Mandatory = $true)]
    [string]$InputPath,
    [Parameter(Mandatory = $true)]
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'

function Convert-HexByte {
    param([string]$Text, [int]$Offset)
    return [Convert]::ToByte($Text.Substring($Offset, 2), 16)
}

$resolvedInput = (Resolve-Path -LiteralPath $InputPath).Path
$records = New-Object 'System.Collections.Generic.List[object]'
[uint32]$baseAddress = 0
[uint32]$maximumAddress = 0
$sawEndRecord = $false

foreach ($rawLine in [IO.File]::ReadLines($resolvedInput)) {
    $line = $rawLine.Trim()
    if ($line.Length -eq 0) { continue }
    if (-not $line.StartsWith(':')) { throw "Invalid Intel HEX line: $line" }

    $hex = $line.Substring(1)
    if (($hex.Length % 2) -ne 0 -or $hex.Length -lt 10) {
        throw "Invalid Intel HEX record length: $line"
    }

    $bytes = New-Object byte[] ($hex.Length / 2)
    for ($i = 0; $i -lt $bytes.Length; ++$i) {
        $bytes[$i] = Convert-HexByte $hex ($i * 2)
    }

    $length = $bytes[0]
    if ($bytes.Length -ne ($length + 5)) {
        throw "Byte count does not match record length: $line"
    }

    [int]$sum = 0
    foreach ($value in $bytes) { $sum = ($sum + $value) -band 0xFF }
    if ($sum -ne 0) { throw "Checksum error in Intel HEX record: $line" }

    [uint32]$address = ([uint32]$bytes[1] -shl 8) -bor $bytes[2]
    $type = $bytes[3]

    switch ($type) {
        0 {
            [uint32]$absoluteAddress = $baseAddress + $address
            $data = New-Object byte[] $length
            [Array]::Copy($bytes, 4, $data, 0, $length)
            $records.Add([pscustomobject]@{ Address = $absoluteAddress; Data = $data })
            [uint32]$endAddress = $absoluteAddress + $length
            if ($endAddress -gt $maximumAddress) { $maximumAddress = $endAddress }
        }
        1 { $sawEndRecord = $true }
        2 {
            if ($length -ne 2) { throw 'Invalid extended segment address record.' }
            $baseAddress = (([uint32]$bytes[4] -shl 8) -bor $bytes[5]) -shl 4
        }
        4 {
            if ($length -ne 2) { throw 'Invalid extended linear address record.' }
            $baseAddress = (([uint32]$bytes[4] -shl 8) -bor $bytes[5]) -shl 16
        }
        3 { }
        5 { }
        default { throw "Unsupported Intel HEX record type: $type" }
    }
}

if (-not $sawEndRecord) { throw 'Intel HEX end-of-file record is missing.' }
if ($maximumAddress -eq 0) { throw 'Intel HEX file contains no data records.' }
if ($maximumAddress -gt [int]::MaxValue) { throw 'Output image is too large.' }

$image = New-Object byte[] ([int]$maximumAddress)
foreach ($record in $records) {
    [Array]::Copy($record.Data, 0, $image, [int]$record.Address, $record.Data.Length)
}

$outputDirectory = Split-Path -Parent $OutputPath
if ($outputDirectory -and -not (Test-Path -LiteralPath $outputDirectory)) {
    New-Item -ItemType Directory -Path $outputDirectory | Out-Null
}
[IO.File]::WriteAllBytes($OutputPath, $image)
Write-Output "Converted $resolvedInput to $OutputPath ($($image.Length) bytes)."
