param(
    [Parameter(Mandatory = $true)]
    [string]$SourceEnvAmbient,

    [Parameter(Mandatory = $true)]
    [string]$SourceBaseSounds,

    [string]$SourcePatchSounds = '',

    [string]$OutputRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

function Assert-File([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Required file is missing: $Path"
    }
}

function Assert-Directory([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        throw "Required directory is missing: $Path"
    }
}

function Get-SectionBody([string]$Text, [string]$SectionName) {
    $name = [regex]::Escape($SectionName)
    $match = [regex]::Match(
        $Text,
        "(?ms)^\[$name\][^\r\n]*\r?\n(.*?)(?=^\[|\z)"
    )
    if (-not $match.Success) {
        throw "LAEE ambient section is missing: $SectionName"
    }
    return $match.Groups[1].Value
}

function Get-SectionField([string]$Body, [string]$FieldName) {
    $name = [regex]::Escape($FieldName)
    $match = [regex]::Match(
        $Body,
        "(?m)^\s*$name\s*=\s*([^;\r\n]+)"
    )
    if (-not $match.Success) {
        throw "Field '$FieldName' is missing in a selected LAEE section"
    }
    return $match.Groups[1].Value.Trim()
}

function Parse-Pair([string]$Value, [string]$FieldName) {
    $parts = @($Value.Split(',') | ForEach-Object { $_.Trim() })
    if ($parts.Count -ne 2) {
        throw "Field '$FieldName' is not a pair: $Value"
    }
    return @([double]::Parse($parts[0], [Globalization.CultureInfo]::InvariantCulture),
             [double]::Parse($parts[1], [Globalization.CultureInfo]::InvariantCulture))
}

function Write-Utf8NoBom([string]$Path, [string]$Text) {
    $parent = Split-Path -Parent $Path
    New-Item -ItemType Directory -Path $parent -Force | Out-Null
    [IO.File]::WriteAllText($Path, $Text, (New-Object Text.UTF8Encoding($false)))
}

Assert-File $SourceEnvAmbient
Assert-Directory $SourceBaseSounds
if ($SourcePatchSounds) {
    Assert-Directory $SourcePatchSounds
}

$sourceText = Get-Content -LiteralPath $SourceEnvAmbient -Raw
$sourceChannels = [ordered]@{
    night       = 'ambient_env_night'
    morning     = 'ambient_env_morning'
    day         = 'ambient_env_day'
    evening     = 'ambient_env_evening'
    rain        = 'ambient_env_rain'
    thunder     = 'ambient_env_thunder'
    tuman       = 'ambient_env_tuman'
    tuman_night = 'ambient_env_tuman_night'
}

$parsedChannels = [ordered]@{}
$allSounds = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)

foreach ($entry in $sourceChannels.GetEnumerator()) {
    $body = Get-SectionBody $sourceText $entry.Value
    $period = Parse-Pair (Get-SectionField $body 'sound_period') 'sound_period'
    $distance = Parse-Pair (Get-SectionField $body 'sound_dist') 'sound_dist'
    $sounds = @(
        (Get-SectionField $body 'sounds').Split(',') |
            ForEach-Object { $_.Trim() } |
            Where-Object { $_ } |
            Where-Object {
                # The player explicitly rejected close fly/buzz samples in the
                # preceding IWP test. Keep LAEE's soundscape but omit its seven
                # rnd_fly/rnd_flies one-shots from this test build.
                $_ -notmatch '(?i)\\rnd_fl(?:y|ies)(?:\d|_|$)'
            }
    )
    if ($sounds.Count -eq 0) {
        throw "Selected LAEE section became empty: $($entry.Value)"
    }
    foreach ($sound in $sounds) {
        [void]$allSounds.Add($sound)
    }
    $parsedChannels[$entry.Key] = [pscustomobject]@{
        SourceSection = $entry.Value
        PeriodMinMs   = [int][Math]::Round($period[0] * 1000)
        PeriodMaxMs   = [int][Math]::Round($period[1] * 1000)
        MinDistance   = $distance[0]
        MaxDistance   = $distance[1]
        Sounds        = $sounds
    }
}

$soundOutputRoot = Join-Path $OutputRoot 'sounds\pf_laee_ambient'
$manifestRows = New-Object 'System.Collections.Generic.List[string]'

foreach ($sound in @($allSounds | Sort-Object)) {
    $relativeOgg = $sound + '.ogg'
    $patchSource = if ($SourcePatchSounds) {
        Join-Path $SourcePatchSounds $relativeOgg
    } else {
        ''
    }
    $baseSource = Join-Path $SourceBaseSounds $relativeOgg
    if ($patchSource -and (Test-Path -LiteralPath $patchSource -PathType Leaf)) {
        $source = $patchSource
        $origin = 'LAEE patch'
    } elseif (Test-Path -LiteralPath $baseSource -PathType Leaf) {
        $source = $baseSource
        $origin = 'LAEE base'
    } else {
        throw "Referenced LAEE sound is missing: $relativeOgg"
    }

    $destination = Join-Path $soundOutputRoot $relativeOgg
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination -Force
    $sourceHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
    $destinationHash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
    if ($sourceHash -ne $destinationHash) {
        throw "Copied sound hash mismatch: $relativeOgg"
    }
    $manifestRows.Add("$sound|$origin|$sourceHash")
}

$channelLines = New-Object 'System.Collections.Generic.List[string]'
$channelLines.Add('; TEST 5: selected LAEE outdoor ambient pools for IX-Ray Pripyat.')
$channelLines.Add('; Section names and sound paths are private to avoid DLTX collisions.')
$channelLines.Add('; LAEE rnd_fly/rnd_flies samples are deliberately excluded.')
$channelLines.Add('')

foreach ($entry in $parsedChannels.GetEnumerator()) {
    $channel = $entry.Value
    $channelLines.Add("[pf_laee_pripyat_$($entry.Key)]")
    $channelLines.Add(('max_distance = {0:F6}' -f $channel.MaxDistance))
    $channelLines.Add(('min_distance = {0:F6}' -f $channel.MinDistance))
    $channelLines.Add("period0 = $($channel.PeriodMinMs)")
    $channelLines.Add("period1 = $($channel.PeriodMaxMs)")
    $channelLines.Add("period2 = $($channel.PeriodMinMs)")
    $channelLines.Add("period3 = $($channel.PeriodMaxMs)")
    $prefixed = @($channel.Sounds | ForEach-Object { 'pf_laee_ambient\' + $_ })
    $channelLines.Add('sounds = ' + ($prefixed -join ', '))
    $channelLines.Add('')
}

$channelPath = Join-Path $OutputRoot 'configs\environment\mod_sound_channels_pf_laee_pripyat_test.ltx'
Write-Utf8NoBom $channelPath (($channelLines -join "`r`n") + "`r`n")

$standardEffects = 'da_pripyat_effect_1, da_pripyat_effect_2, da_pripyat_effect_3, da_pripyat_effect_4, da_pripyat_effect_5, da_pripyat_effect_6, da_pripyat_effect_7, da_pripyat_effect_8'
$profile = @(
    [pscustomobject]@{ Name='night';       Effects=$standardEffects; Max=80; Min=30; Channel='night' },
    [pscustomobject]@{ Name='morning';     Effects=$standardEffects; Max=50; Min=20; Channel='morning' },
    [pscustomobject]@{ Name='day';         Effects=$standardEffects; Max=50; Min=20; Channel='day' },
    [pscustomobject]@{ Name='evening';     Effects=$standardEffects; Max=50; Min=20; Channel='evening' },
    [pscustomobject]@{ Name='rain';        Effects=$standardEffects; Max=50; Min=10; Channel='rain' },
    [pscustomobject]@{ Name='rain_day';    Effects=$standardEffects; Max=40; Min=20; Channel='rain' },
    [pscustomobject]@{ Name='rain_night';  Effects=$standardEffects; Max=60; Min=30; Channel='rain' },
    [pscustomobject]@{ Name='pre_storm';   Effects=$standardEffects; Max=20; Min=10; Channel='thunder' },
    [pscustomobject]@{ Name='storm_day';   Effects=$standardEffects; Max=20; Min=15; Channel='thunder' },
    [pscustomobject]@{ Name='storm_night'; Effects=$standardEffects; Max=30; Min=15; Channel='thunder' },
    [pscustomobject]@{ Name='tuman';       Effects=$standardEffects; Max=50; Min=10; Channel='tuman' },
    [pscustomobject]@{ Name='tuman_day';   Effects=$standardEffects; Max=40; Min=20; Channel='tuman' },
    [pscustomobject]@{ Name='tuman_night'; Effects=$standardEffects; Max=60; Min=30; Channel='tuman_night' },
    [pscustomobject]@{ Name='dark';        Effects='effect_1, effect_storm'; Max=60; Min=10; Channel='tuman_night' },
    [pscustomobject]@{ Name='fog';         Effects='effect_1, effect_2, effect_3, effect_4, effect_5, effect_6, effect_7, effect_8, effect_9'; Max=60; Min=10; Channel='tuman' },
    [pscustomobject]@{ Name='th';          Effects='effect_storm'; Max=5; Min=1; Channel='thunder' }
)

$profileLines = New-Object 'System.Collections.Generic.List[string]'
$profileLines.Add('; TEST 5: LAEE sound pools mapped onto IX-Ray/Paradox Pripyat states.')
$profileLines.Add('; Weather visuals and effect sections remain owned by the active IX-Ray setup.')
$profileLines.Add('')
foreach ($section in $profile) {
    $profileLines.Add("[$($section.Name)]")
    $profileLines.Add("effects = $($section.Effects)")
    $profileLines.Add(('max_effect_period = {0:F6}' -f [double]$section.Max))
    $profileLines.Add(('min_effect_period = {0:F6}' -f [double]$section.Min))
    $profileLines.Add("sound_channels = pf_laee_pripyat_$($section.Channel)")
    $profileLines.Add('')
}

$profilePath = Join-Path $OutputRoot 'configs\environment\ambients\pripyat_full.ltx'
Write-Utf8NoBom $profilePath (($profileLines -join "`r`n") + "`r`n")

$manifestPath = Join-Path $OutputRoot 'tools\selected_sounds_manifest.txt'
Write-Utf8NoBom $manifestPath ((@(
    '# relative sound path|source layer|SHA-256',
    '# Generated from installed Lost Alpha Enhanced Edition.'
) + @($manifestRows)) -join "`r`n" + "`r`n")

[pscustomobject]@{
    OutputRoot    = $OutputRoot
    Channels      = $parsedChannels.Count
    UniqueSounds  = $allSounds.Count
    ChannelConfig = $channelPath
    AmbientConfig = $profilePath
    Manifest      = $manifestPath
}
