using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.IO;
using UnityEditor;

public class ExtractMeshByBinaryWithNormal : MonoBehaviour
{
    private BinaryWriter binaryWriter = null;
    private int m_nFrames = 0;

    string GetAlbedoTextureFileName(Material mat)
    {
        if (!mat) return "null";

        Texture tex = null;

        if (mat.HasProperty("_BaseMap"))
            tex = mat.GetTexture("_BaseMap");
        else if (mat.HasProperty("_MainTex"))
            tex = mat.GetTexture("_MainTex");

        if (!tex) return "null";

#if UNITY_EDITOR
    string assetPath = AssetDatabase.GetAssetPath(tex);
    if (string.IsNullOrEmpty(assetPath)) return "null";

    // "banana.png" -> "banana.dds"
    string baseName = Path.GetFileNameWithoutExtension(assetPath);
    baseName = baseName.Replace(" ", "_");
    return baseName + ".dds";
#else
        return "null";
#endif
    }

    void WriteObjectName(Object obj)
    {
        binaryWriter.Write((obj) ? string.Copy(obj.name).Replace(" ", "_") : "null");
    }

    void WriteObjectName(string strHeader, int i, Object obj)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(i);
        binaryWriter.Write((obj) ? string.Copy(obj.name).Replace(" ", "_") : "null");
    }

    void WriteString(string strToWrite)
    {
        binaryWriter.Write(strToWrite);
    }

    void WriteString(string strHeader, string strToWrite)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(strToWrite);
    }

    void WriteInteger(string strHeader, int i)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(i);
    }

    void WriteFloat(string strHeader, float f)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(f);
    }

    void WriteVector(Vector2 v)
    {
        binaryWriter.Write(v.x);
        binaryWriter.Write(v.y);
    }

    void WriteVector(Vector3 v)
    {
        binaryWriter.Write(v.x);
        binaryWriter.Write(v.y);
        binaryWriter.Write(v.z);
    }

    void WriteVector(Vector4 v)
    {
        binaryWriter.Write(v.x);
        binaryWriter.Write(v.y);
        binaryWriter.Write(v.z);
        binaryWriter.Write(v.w);
    }

    void WriteColor(Color c)
    {
        binaryWriter.Write(c.r);
        binaryWriter.Write(c.g);
        binaryWriter.Write(c.b);
        binaryWriter.Write(c.a);
    }

    void WriteVectors(string strHeader, Vector2[] vectors)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(vectors.Length);
        if (vectors.Length > 0)
            foreach (Vector2 v in vectors) WriteVector(v);
    }

    void WriteVectors(string strHeader, Vector3[] vectors)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(vectors.Length);
        if (vectors.Length > 0)
            foreach (Vector3 v in vectors) WriteVector(v);
    }

    void WriteColors(string strHeader, Color[] colors)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(colors.Length);
        if (colors.Length > 0)
            foreach (Color c in colors) WriteColor(c);
    }

    void WriteIntegers(string strHeader, int n, int[] pIntegers)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(n);
        binaryWriter.Write(pIntegers.Length);
        if (pIntegers.Length > 0)
            foreach (int i in pIntegers) binaryWriter.Write(i);
    }

    void WriteBoundingBox(string strHeader, Bounds bounds)
    {
        binaryWriter.Write(strHeader);
        WriteVector(bounds.center);
        WriteVector(bounds.extents);
    }

    void WriteMatrix(Matrix4x4 matrix)
    {
        binaryWriter.Write(matrix.m00); binaryWriter.Write(matrix.m10); binaryWriter.Write(matrix.m20); binaryWriter.Write(matrix.m30);
        binaryWriter.Write(matrix.m01); binaryWriter.Write(matrix.m11); binaryWriter.Write(matrix.m21); binaryWriter.Write(matrix.m31);
        binaryWriter.Write(matrix.m02); binaryWriter.Write(matrix.m12); binaryWriter.Write(matrix.m22); binaryWriter.Write(matrix.m32);
        binaryWriter.Write(matrix.m03); binaryWriter.Write(matrix.m13); binaryWriter.Write(matrix.m23); binaryWriter.Write(matrix.m33);
    }

    void WriteTransform(string strHeader, Transform current)
    {
        binaryWriter.Write(strHeader);
        WriteVector(current.localPosition);
        WriteVector(current.localEulerAngles);
        WriteVector(current.localScale);
        WriteVector(current.localRotation);
    }


    void WriteMeshInfo(Mesh mesh)
    {
        binaryWriter.Write("<Mesh>:");
        binaryWriter.Write(mesh.vertexCount);
        WriteObjectName(mesh);

        WriteBoundingBox("<Bounds>:", mesh.bounds);

        WriteVectors("<Positions>:", mesh.vertices);
        WriteColors("<Colors>:", mesh.colors);
        WriteVectors("<Normals>:", mesh.normals);


        WriteVectors("<UV0>:", mesh.uv);

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


    void WriteMaterials(Material[] materials)
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


            string texFile = GetAlbedoTextureFileName(materials[i]);
            WriteString("<AlbedoTexture>:", texFile);
        }

        WriteString("</Materials>");
    }

    void WriteFrameInfo(Transform current)
    {
        if (!current.gameObject.activeSelf) return;

        WriteObjectName("<Frame>:", m_nFrames++, current.gameObject);

        WriteTransform("<Transform>:", current);

        MeshFilter meshFilter = current.GetComponent<MeshFilter>();
        MeshRenderer meshRenderer = current.GetComponent<MeshRenderer>();

        if (meshFilter && meshRenderer)
        {
            WriteMeshInfo(meshFilter.sharedMesh);

            Material[] materials = meshRenderer.materials;
            if (materials.Length > 0)
                WriteMaterials(materials);
        }
    }

    void WriteFrameHierarchyInfo(Transform child)
    {
        WriteFrameInfo(child);

        WriteInteger("<Children>:", child.childCount);

        for (int k = 0; k < child.childCount; k++)
            WriteFrameHierarchyInfo(child.GetChild(k));

        WriteString("</Frame>");
    }

    void Start()
    {
        binaryWriter = new BinaryWriter(
            File.Open(string.Copy(gameObject.name).Replace(" ", "_") + ".bin", FileMode.Create)
        );

        WriteString("<Hierarchy>:");
        WriteFrameHierarchyInfo(transform);
        WriteString("</Hierarchy>");

        binaryWriter.Flush();
        binaryWriter.Close();

        Debug.Log("Model Binary Write Completed (UV + Texture Included)");
    }
}