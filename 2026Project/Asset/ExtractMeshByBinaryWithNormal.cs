// Place this script under: Assets/Editor/
// Usage:
// 1) Attach this component to the root GameObject you want to export.
// 2) In Inspector, set Game Root Path to the folder your DX12 game EXE uses as Working Directory
//    (the folder that contains the 'Model' folder and 'Asset/DDS_File' folder).
// 3) Set Texconv Exe Path to texconv.exe (DirectXTex).
// 4) Click "Export BIN + DDS" button.
//
// It will:
// - Export .bin to <GameRoot>/Model/<GameObjectName>.bin
// - Convert albedo texture(s) to DDS and output to <GameRoot>/Asset/DDS_File/
// - Write <AlbedoTexture>: <name>.dds into the .bin
//
// Notes:
// - Albedo uses BC7_UNORM_SRGB by default.
// - If your texture source file is NOT a PNG/JPG/TGA/etc on disk (e.g. generated at runtime), DDS conversion will be skipped.
// - If you have multiple textures with the same file name in different folders, names may collide.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using UnityEngine;

#if UNITY_EDITOR
using UnityEditor;
#endif

public class ExtractMeshByBinaryWithNormal : MonoBehaviour
{
    // ========================
    // Inspector settings
    // ========================

    [Header("Game output (DX12 project)")]
    [Tooltip("Folder that contains 'Model' and 'Asset' folders used by your DX12 game at runtime (Working Directory).")]
    public string gameRootPath = "";

    [Tooltip("Relative folder name for .bin files under Game Root Path.")]
    public string modelFolderName = "Model";

    [Tooltip("Relative folder for DDS textures under Game Root Path.")]
    public string ddsFolderRelative = "Asset/DDS_File";

    [Header("DDS conversion (texconv)")]
    public bool convertTexturesToDDS = true;

    [Tooltip("Full path to texconv.exe (from Microsoft DirectXTex). Example: C:/Tools/DirectXTex/texconv.exe")]
    public string texconvExePath = "C:\\Users\\PI\\My project\\Assets\\Texconv.exe";

    [Tooltip("Overwrite existing DDS files.")]
    public bool overwriteDDS = true;

    [Tooltip("Generate mipmaps (-m 0).")]
    public bool generateMipMaps = true;

    [Tooltip("Use sRGB format for albedo (BC7_UNORM_SRGB). If unchecked, uses BC7_UNORM.")]
    public bool albedoIsSRGB = true;

    // ========================
    // Internal state
    // ========================

    private BinaryWriter binaryWriter = null;
    private int m_nFrames = 0;

#if UNITY_EDITOR
    // assetPath -> ddsFileName (just the file name, no directories)
    private readonly Dictionary<string, string> _albedoAssetPathToDdsName = new Dictionary<string, string>();
#endif


#if UNITY_EDITOR
    // ------------------------
    // UI Button
    // ------------------------
    public void ExportBinAndDDS()
    {
        try
        {
            m_nFrames = 0;
#if UNITY_EDITOR
            _albedoAssetPathToDdsName.Clear();
#endif

            string root = ResolveGameRootPath();
            string modelOutDir = Path.Combine(root, modelFolderName);
            string ddsOutDir = Path.Combine(root, ddsFolderRelative);

            Directory.CreateDirectory(modelOutDir);
            Directory.CreateDirectory(ddsOutDir);

            string binFileName = SanitizeFileName(gameObject.name) + ".bin";
            string binPath = Path.Combine(modelOutDir, binFileName);

            using (binaryWriter = new BinaryWriter(File.Open(binPath, FileMode.Create, FileAccess.Write)))
            {
                WriteString("<Hierarchy>:");
                WriteFrameHierarchyInfo(transform);
                WriteString("</Hierarchy>");
                binaryWriter.Flush();
            }

            binaryWriter = null;

            if (convertTexturesToDDS)
            {
                ConvertAllRegisteredAlbedoTexturesToDDS(ddsOutDir);
            }

            UnityEngine.Debug.Log($"Export Completed:\nBIN: {binPath}\nDDS Folder: {ddsOutDir}");
        }
        catch (Exception e)
        {
            UnityEngine.Debug.LogError("ExportBinAndDDS failed: " + e);
        }
        finally
        {
            if (binaryWriter != null)
            {
                try { binaryWriter.Close(); } catch { /* ignore */ }
                binaryWriter = null;
            }
        }
    }

    private string ResolveGameRootPath()
    {
        // If user didn't set it, export next to the Unity project (../ExportedGameRoot)
        if (!string.IsNullOrWhiteSpace(gameRootPath))
        {
            return Path.GetFullPath(gameRootPath);
        }

        string unityProjectRoot = Directory.GetParent(Application.dataPath).FullName;
        string fallback = Path.Combine(unityProjectRoot, "ExportedGameRoot");
        return Path.GetFullPath(fallback);
    }

    private void ConvertAllRegisteredAlbedoTexturesToDDS(string ddsOutDir)
    {
        string texconv = ResolveTexconvPath();
        if (string.IsNullOrEmpty(texconv) || !File.Exists(texconv))
        {
            UnityEngine.Debug.LogError("texconv.exe not found. Set 'Texconv Exe Path' in Inspector.");
            return;
        }

        foreach (var kv in _albedoAssetPathToDdsName)
        {
            string assetPath = kv.Key;      // e.g. Assets/Textures/banana.png
            string expectedDdsName = kv.Value; // e.g. banana.dds (spaces->_ already applied)

            string absSrc = ToAbsoluteAssetPath(assetPath);
            if (string.IsNullOrEmpty(absSrc) || !File.Exists(absSrc))
            {
                UnityEngine.Debug.LogWarning($"[DDS SKIP] Source not found on disk: {assetPath}");
                continue;
            }

            // Build args
            string format = albedoIsSRGB ? "BC7_UNORM_SRGB" : "BC7_UNORM";
            string mips = generateMipMaps ? "-m 0" : "";
            string overwrite = overwriteDDS ? "-y" : "";

            string args = $"{overwrite} -f {format} {mips} -o \"{ddsOutDir}\" \"{absSrc}\"";

            int exit = RunProcess(texconv, args, out string stdOut, out string stdErr);
            if (exit != 0)
            {
                UnityEngine.Debug.LogError($"[DDS FAIL] texconv exit={exit}\nArgs: {args}\nOUT: {stdOut}\nERR: {stdErr}");
                continue;
            }

            // texconv outputs with the source base file name (may contain spaces) and extension .DDS
            // But our engine/bin stores sanitized name (spaces -> underscore). Rename if needed.
            string producedBase = Path.GetFileNameWithoutExtension(absSrc);
            string producedCandidate1 = Path.Combine(ddsOutDir, producedBase + ".DDS");
            string producedCandidate2 = Path.Combine(ddsOutDir, producedBase + ".dds");

            string expectedPath = Path.Combine(ddsOutDir, expectedDdsName);

            if (!File.Exists(expectedPath))
            {
                string producedPath = null;

                if (File.Exists(producedCandidate1)) producedPath = producedCandidate1;
                else if (File.Exists(producedCandidate2)) producedPath = producedCandidate2;
                else
                {
                    // Last resort: try to find by base name ignoring extension case
                    string[] matches = Directory.GetFiles(ddsOutDir, producedBase + ".*", SearchOption.TopDirectoryOnly);
                    foreach (var m in matches)
                    {
                        if (m.EndsWith(".dds", StringComparison.OrdinalIgnoreCase))
                        {
                            producedPath = m;
                            break;
                        }
                    }
                }

                if (!string.IsNullOrEmpty(producedPath) && File.Exists(producedPath))
                {
                    try
                    {
                        // If expected exists and overwrite is enabled, delete
                        if (File.Exists(expectedPath) && overwriteDDS) File.Delete(expectedPath);

                        // Rename/move
                        File.Move(producedPath, expectedPath);
                    }
                    catch (Exception e)
                    {
                        UnityEngine.Debug.LogWarning($"[DDS WARN] Could not rename '{producedPath}' -> '{expectedPath}': {e.Message}");
                    }
                }
            }
        }
    }

    private string ResolveTexconvPath()
    {
        if (!string.IsNullOrWhiteSpace(texconvExePath))
            return Path.GetFullPath(texconvExePath);

        // Try some common locations relative to Unity project
        string projectRoot = Directory.GetParent(Application.dataPath).FullName;
        string[] candidates =
        {
            Path.Combine(projectRoot, "Tools", "texconv.exe"),
            Path.Combine(projectRoot, "texconv.exe"),
            Path.Combine(Application.dataPath, "Tools", "texconv.exe"),
        };

        foreach (var c in candidates)
        {
            if (File.Exists(c)) return c;
        }

        return "";
    }

    private static int RunProcess(string exePath, string args, out string stdOut, out string stdErr)
    {
        var psi = new ProcessStartInfo
        {
            FileName = exePath,
            Arguments = args,
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = true,
        };

        using (var p = Process.Start(psi))
        {
            stdOut = p.StandardOutput.ReadToEnd();
            stdErr = p.StandardError.ReadToEnd();
            p.WaitForExit();
            return p.ExitCode;
        }
    }

    private static string SanitizeFileName(string name)
    {
        if (string.IsNullOrEmpty(name)) return "unnamed";
        name = name.Replace(" ", "_");
        foreach (char c in Path.GetInvalidFileNameChars())
            name = name.Replace(c, '_');
        return name;
    }

    private static string ToAbsoluteAssetPath(string assetPath)
    {
        if (string.IsNullOrEmpty(assetPath)) return "";

        // assetPath is like "Assets/..."
        string projectRoot = Directory.GetParent(Application.dataPath).FullName;
        return Path.GetFullPath(Path.Combine(projectRoot, assetPath));
    }
#endif


    // ========================
    // BIN writing helpers
    // ========================

    private void WriteObjectName(UnityEngine.Object obj)
    {
        binaryWriter.Write((obj) ? string.Copy(obj.name).Replace(" ", "_") : "null");
    }

    private void WriteObjectName(string strHeader, int i, UnityEngine.Object obj)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(i);
        binaryWriter.Write((obj) ? string.Copy(obj.name).Replace(" ", "_") : "null");
    }

    private void WriteString(string strToWrite)
    {
        binaryWriter.Write(strToWrite);
    }

    private void WriteString(string strHeader, string strToWrite)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(strToWrite);
    }

    private void WriteInteger(string strHeader, int i)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(i);
    }

    private void WriteFloat(string strHeader, float f)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(f);
    }

    private void WriteVector(Quaternion q)
    {
        binaryWriter.Write(q.x);
        binaryWriter.Write(q.y);
        binaryWriter.Write(q.z);
        binaryWriter.Write(q.w);
    }

    private void WriteColor(string strHeader, Color c)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(c.r);
        binaryWriter.Write(c.g);
        binaryWriter.Write(c.b);
        binaryWriter.Write(c.a);
    }
    private void WriteVector(Vector2 v)
    {
        binaryWriter.Write(v.x);
        binaryWriter.Write(v.y);
    }

    private void WriteVector(Vector3 v)
    {
        binaryWriter.Write(v.x);
        binaryWriter.Write(v.y);
        binaryWriter.Write(v.z);
    }

    private void WriteVector(Vector4 v)
    {
        binaryWriter.Write(v.x);
        binaryWriter.Write(v.y);
        binaryWriter.Write(v.z);
        binaryWriter.Write(v.w);
    }

    private void WriteColor(Color c)
    {
        binaryWriter.Write(c.r);
        binaryWriter.Write(c.g);
        binaryWriter.Write(c.b);
        binaryWriter.Write(c.a);
    }

    private void WriteVectors(string strHeader, Vector2[] vectors)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(vectors.Length);
        if (vectors.Length > 0)
            foreach (Vector2 v in vectors) WriteVector(v);
    }

    private void WriteVectors(string strHeader, Vector3[] vectors)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(vectors.Length);
        if (vectors.Length > 0)
            foreach (Vector3 v in vectors) WriteVector(v);
    }

    private void WriteColors(string strHeader, Color[] colors)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(colors.Length);
        if (colors.Length > 0)
            foreach (Color c in colors) WriteColor(c);
    }

    private void WriteIntegers(string strHeader, int n, int[] pIntegers)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(n);
        binaryWriter.Write(pIntegers.Length);
        if (pIntegers.Length > 0)
            foreach (int i in pIntegers) binaryWriter.Write(i);
    }

    private void WriteBoundingBox(string strHeader, Bounds bounds)
    {
        binaryWriter.Write(strHeader);
        WriteVector(bounds.center);
        WriteVector(bounds.extents);
    }

    private void WriteTransform(string strHeader, Transform current)
    {
        binaryWriter.Write(strHeader);
        WriteVector(current.localPosition);
        WriteVector(current.localEulerAngles);
        WriteVector(current.localScale);
        WriteVector(current.localRotation);
    }

    private void WriteMeshInfo(Mesh mesh)
    {
        binaryWriter.Write("<Mesh>:");
        binaryWriter.Write(mesh.vertexCount);
        WriteObjectName(mesh);

        WriteBoundingBox("<Bounds>:", mesh.bounds);

        WriteVectors("<Positions>:", mesh.vertices);
        WriteColors("<Colors>:", mesh.colors);
        WriteVectors("<Normals>:", mesh.normals);

        Vector2[] uvs = mesh.uv;
        for (int i = 0; i < uvs.Length; i++) { uvs[i].y = 1.0f - uvs[i].y; }
        WriteVectors("<UV0>:", uvs);

        binaryWriter.Write("<SubMeshes>:");
        binaryWriter.Write(mesh.subMeshCount);

        if (mesh.subMeshCount > 0)
        {
            for (int i = 0; i < mesh.subMeshCount; i++)
            {
                int[] subindicies = mesh.GetTriangles(i);
                WriteIntegers("<SubMesh>:", i, subindicies);
            }
        }

        binaryWriter.Write("</Mesh>");
    }

    private void WriteMaterials(Material[] materials, string objName)
    {
        WriteInteger("<Materials>:", materials.Length);

        for (int i = 0; i < materials.Length; i++)
        {
            WriteInteger("<Material>:", i);

            if (materials[i].HasProperty("_Color"))
                WriteColor("<AlbedoColor>:", materials[i].GetColor("_Color"));

            if (materials[i].HasProperty("_EmissionColor"))
                WriteColor("<EmissiveColor>:", materials[i].GetColor("_EmissionColor"));

            if (materials[i].HasProperty("_Metallic"))
                WriteFloat("<Metallic>:", materials[i].GetFloat("_Metallic"));

            if (materials[i].HasProperty("_Glossiness"))
                WriteFloat("<Glossiness>:", materials[i].GetFloat("_Glossiness"));

            string texFile = GetAlbedoTextureDDSFileName(materials[i], objName);
            WriteString("<AlbedoTexture>:", texFile);
        }

        WriteString("</Materials>");
    }

    private void WriteFrameInfo(Transform current)
    {
        if (!current.gameObject.activeSelf) return;

        WriteObjectName("<Frame>:", m_nFrames++, current.gameObject);

        WriteTransform("<Transform>:", current);

        MeshFilter meshFilter = current.GetComponent<MeshFilter>();
        MeshRenderer meshRenderer = current.GetComponent<MeshRenderer>();

        if (meshFilter && meshRenderer)
        {
            WriteMeshInfo(meshFilter.sharedMesh);

            Material[] materials = meshRenderer.sharedMaterials;
            if (materials != null && materials.Length > 0)
                WriteMaterials(materials, current.name);
        }
    }

    private void WriteFrameHierarchyInfo(Transform child)
    {
        WriteFrameInfo(child);

        WriteInteger("<Children>:", child.childCount);

        for (int k = 0; k < child.childCount; k++)
            WriteFrameHierarchyInfo(child.GetChild(k));

        WriteString("</Frame>");
    }

    // ------------------------
    // Texture name + register
    // ------------------------
    private string GetAlbedoTextureDDSFileName(Material mat, string objName)
    {
        if (!mat) return "null";

        Texture tex = null;
        if (mat.HasProperty("_BaseMap"))
            tex = mat.GetTexture("_BaseMap");
        else if (mat.HasProperty("_MainTex"))
            tex = mat.GetTexture("_MainTex");
        else if (mat.HasProperty("_BaseColorMap")) // HDRP fallback
            tex = mat.GetTexture("_BaseColorMap");

        if (!tex) return "null";

#if UNITY_EDITOR
        string assetPath = AssetDatabase.GetAssetPath(tex);
        if (string.IsNullOrEmpty(assetPath))
            return "null";

        if(_albedoAssetPathToDdsName.TryGetValue(assetPath, out string existingDdsName))
        {
            return existingDdsName;
        }

        // Make file name deterministic and safe
        string baseName = Path.GetFileNameWithoutExtension(assetPath);
        baseName = SanitizeFileName(baseName);
        string safeObjName = SanitizeFileName(gameObject.name);

        string ddsName = $"{safeObjName}_{baseName}.dds";

        // Register for conversion
        //if (!_albedoAssetPathToDdsName.ContainsKey(assetPath))
        //    _albedoAssetPathToDdsName.Add(assetPath, ddsName);

        _albedoAssetPathToDdsName.Add(assetPath, ddsName);

        return ddsName;
#else
        return "null";
#endif
    }
}


#if UNITY_EDITOR
// Custom inspector with an Export button
[CustomEditor(typeof(ExtractMeshByBinaryWithNormal))]
public class ExtractMeshByBinaryWithNormalEditor : Editor
{
    public override void OnInspectorGUI()
    {
        DrawDefaultInspector();

        GUILayout.Space(10);

        var exporter = (ExtractMeshByBinaryWithNormal)target;

        if (GUILayout.Button("Export BIN + DDS (texconv)"))
        {
            exporter.ExportBinAndDDS();
        }

        GUILayout.Space(6);
        EditorGUILayout.HelpBox(
            "Export outputs to:\n" +
            "- <GameRoot>/Model/<ObjectName>.bin\n" +
            "- <GameRoot>/Asset/DDS_File/<TextureName>.dds\n\n" +
            "Make sure your DX12 game's Working Directory points to the same GameRoot.",
            MessageType.Info);
    }
}
#endif
