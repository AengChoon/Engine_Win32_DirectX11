﻿#include "Model.h"
#include "Bindables.h"
#include "Camera.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

Mesh::Mesh(const Graphics& InGraphics, std::vector<std::unique_ptr<Bindable>>&& InBindables)
{
	if (!IsStaticInitialized())
	{
		AddStaticBindable(std::make_unique<Topology>(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));
	}

	for (auto& Bindable : InBindables)
	{
		if (auto* MeshIndexBuffer = dynamic_cast<IndexBuffer*>(Bindable.get()))
		{
			AddInstanceIndexBuffer(std::unique_ptr<IndexBuffer>{MeshIndexBuffer});
			Bindable.release();
		}
		else
		{
			AddInstanceBindable(std::move(Bindable));
		}
	}

	AddInstanceBindable(std::make_unique<TransformConstantBuffer>(InGraphics, *this));
}

void Mesh::Draw(const Graphics& InGraphics, const DirectX::XMMATRIX& InAccumulatedTransform) const
{
	DirectX::XMStoreFloat4x4(&TransformMatrix, InAccumulatedTransform);
	Drawable::Draw(InGraphics);
}

DirectX::XMMATRIX Mesh::GetTransformMatrix() const noexcept
{
	return DirectX::XMLoadFloat4x4(&TransformMatrix);
}

Node::Node(std::vector<Mesh*>&& InMeshes, const DirectX::XMMATRIX& InTransformMatrix)
	: Meshes(std::move(InMeshes))
{
	DirectX::XMStoreFloat4x4(&TransformMatrix, InTransformMatrix);
}

void Node::Draw(const Graphics& InGraphics, const DirectX::XMMATRIX& InAccumulatedTransformMatrix) const
{
	const auto MyTransformMatrix = DirectX::XMLoadFloat4x4(&TransformMatrix) * InAccumulatedTransformMatrix;

	for (const auto* Mesh : Meshes)
	{
		Mesh->Draw(InGraphics, MyTransformMatrix);
	}

	for (const auto& Child : Children)
	{
		Child->Draw(InGraphics, MyTransformMatrix);
	}
}

void Node::AddChild(std::unique_ptr<Node>&& InNode)
{
	Children.push_back(std::move(InNode));
}

Model::Model(const Graphics& InGraphics, const std::string_view InFileName)
{
	Assimp::Importer Importer;
	const auto* Scene = Importer.ReadFile(InFileName.data(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

	for (size_t MeshIndex = 0; MeshIndex < Scene->mNumMeshes; ++MeshIndex)
	{
		Meshes.push_back(ParseMesh(InGraphics, *Scene->mMeshes[MeshIndex]));
	}

	Root = ParseNode(*Scene->mRootNode);
}

std::unique_ptr<Mesh> Model::ParseMesh(const Graphics& InGraphics, const aiMesh& InMesh)
{
	DV::VertexBuffer ModelVertexBuffer {{DV::VertexLayout::ElementType::Position3D, DV::VertexLayout::ElementType::Normal}};

	for (unsigned int VertexIndex = 0; VertexIndex < InMesh.mNumVertices; ++VertexIndex)
	{
		ModelVertexBuffer.Emplace
		(
			*reinterpret_cast<DirectX::XMFLOAT3*>(&InMesh.mVertices[VertexIndex]),
			*reinterpret_cast<DirectX::XMFLOAT3*>(&InMesh.mNormals[VertexIndex])
		);
	}

	std::vector<unsigned int> Indices;
	Indices.reserve(InMesh.mNumFaces * 3);
	for (unsigned int FaceIndex = 0; FaceIndex < InMesh.mNumFaces; ++FaceIndex)
	{
		const auto& Face = InMesh.mFaces[FaceIndex];
		assert(Face.mNumIndices == 3);

		for (unsigned int Index = 0; Index < Face.mNumIndices; ++Index)
		{
			Indices.push_back(Face.mIndices[Index]);
		}
	}

	std::vector<std::unique_ptr<Bindable>> Bindables;
	Bindables.push_back(std::make_unique<VertexBuffer>(InGraphics, ModelVertexBuffer));
	Bindables.push_back(std::make_unique<IndexBuffer>(InGraphics, Indices));

	auto ModelVertexShader = std::make_unique<VertexShader>(InGraphics, L"PhongVS.cso");
	auto ModelVertexShaderBlob = ModelVertexShader->GetByteCode();
	Bindables.push_back(std::move(ModelVertexShader));

	Bindables.push_back(std::make_unique<PixelShader>(InGraphics, L"PhongPS.cso"));
	Bindables.push_back(std::make_unique<InputLayout>(InGraphics, ModelVertexBuffer.GetLayout().GetD3D11Layout(), ModelVertexShaderBlob));

	struct PSMaterialConstants
	{
		alignas(16) DirectX::XMFLOAT3 Color {0.6f, 0.6f, 0.8f};
		float SpecularIntensity = 0.6f;
		float SpecularPower = 10.0f;
		float Padding[2];
	} MaterialConstants;

	Bindables.push_back(std::make_unique<PixelConstantBuffer<PSMaterialConstants>>(InGraphics, MaterialConstants, 1u));

	struct PSCameraConstants
	{
		alignas(16) DirectX::XMFLOAT3 Position;
	} CameraConstants;

	CameraConstants.Position = InGraphics.GetCamera().GetPosition();
	Bindables.push_back(std::make_unique<PixelConstantBuffer<PSCameraConstants>>(InGraphics, CameraConstants, 2u));

	return std::make_unique<Mesh>(InGraphics, std::move(Bindables));
}

std::unique_ptr<Node> Model::ParseNode(const aiNode& InNode)
{
	const auto NodeTransformMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(&InNode.mTransformation)));

	std::vector<Mesh*> NodeMeshes;
	NodeMeshes.reserve(InNode.mNumMeshes);

	for (size_t NodeMeshIndex = 0; NodeMeshIndex < InNode.mNumMeshes; ++NodeMeshIndex)
	{
		const auto ModelMeshIndex = InNode.mMeshes[NodeMeshIndex];
		NodeMeshes.push_back(Meshes.at(ModelMeshIndex).get());
	}

	auto pNode = std::make_unique<Node>(std::move(NodeMeshes), NodeTransformMatrix);

	for (size_t ChildrenIndex = 0; ChildrenIndex < InNode.mNumChildren; ++ChildrenIndex)
	{
		pNode->AddChild(ParseNode(*InNode.mChildren[ChildrenIndex]));
	}

	return pNode;
}

void Model::Draw(const Graphics& InGraphics, const DirectX::XMMATRIX& InTransform)
{
	Root->Draw(InGraphics, InTransform);
}