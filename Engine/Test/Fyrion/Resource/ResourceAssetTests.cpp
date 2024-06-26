#include <doctest.h>

#include "Fyrion/Common.hpp"
#include "Fyrion/Resource/ResourceTypes.hpp"
#include "Fyrion/Core/String.hpp"
#include "Fyrion/Resource/Repository.hpp"
#include "Fyrion/IO/FileSystem.hpp"
#include "Fyrion/Engine.hpp"
#include "Fyrion/IO/Path.hpp"
#include "Fyrion/Resource/ResourceAssets.hpp"
#include "Fyrion/Resource/AssetTree.hpp"

using namespace Fyrion;

namespace
{

	struct TxtAsset
	{
        constexpr static u32 Content = 0;
	};

	RID TxtAssetLoadFunction(RID asset, const StringView& path)
	{
		String txt = FileSystem::ReadFileAsString(path);

		RID rid = Repository::CreateResource<TxtAsset>();
		ResourceObject txtAsset = Repository::Write(rid);
		txtAsset.SetValue(TxtAsset::Content, Traits::Move(txt));
		txtAsset.Commit();

		return rid;
	}

	TEST_CASE("Repository::AssetsBasic")
	{
		Engine::Init();
        {
            String assetPath = Path::Join(FY_TEST_FILES, "AssetsBasic");

            if (!FileSystem::GetFileStatus(assetPath).exists)
            {
                return;
            }


            ResourceAssets::AddAssetImporter(".txt", TxtAssetLoadFunction);

            ResourceTypeBuilder<TxtAsset>::Builder()
                .Value<TxtAsset::Content, String>("Content")
                .Build();

            RID root = ResourceAssets::LoadAssetsFromDirectory("Fyrion", assetPath);
            ResourceObject assetRoot = Repository::Read(root);

            Array<RID> directories = assetRoot.GetSubObjectSetAsArray(AssetRoot::Directories);
            CHECK(directories.Size() == 3);

            u32 countRoot = 0;
            u32 countNotRoot = 0;

            for(RID rid : directories)
            {
                ResourceObject directory = Repository::Read(rid);
                RID parent = directory[AssetDirectory::Parent].As<RID>();

                if (parent == root)
                {
                    countRoot++;

                }
                else
                {
                    countNotRoot++;
                }
            }

            CHECK(countRoot == 2);
            CHECK(countNotRoot == 1);

            Array<RID> assets = assetRoot.GetSubObjectSetAsArray(AssetRoot::Assets);
            CHECK(assets.Size() == 3);

            {
                RID rid = Repository::GetByPath("Fyrion://Dir1/Dir1/TxtFile1.txt");
                CHECK(rid);
                ResourceObject txtAsset = Repository::Read(rid);
                CHECK(txtAsset.GetValue<String>(TxtAsset::Content) == "aaaa");
            }

            {
                RID rid = Repository::GetByPath("Fyrion://Dir2/TxtFile2.txt");
                REQUIRE(rid);
                ResourceObject txtAsset = Repository::Read(rid);
                CHECK(txtAsset.GetValue<String>(TxtAsset::Content) == "bbbb");
            }

            {
                AssetTree resourceTree{};
                resourceTree.AddAssetRoot(root);
                resourceTree.Update();

                RID folder = Repository::GetByPath("Fyrion://Dir1/Dir1");

                {
                    Span<AssetNode*> items = resourceTree.GetNode(folder)->nodes;
                    CHECK(items.Size() == 2);
                    CHECK(items[0]->name == "TxtFile1");
                    CHECK(items[0]->parent->rid == folder);
                    CHECK(items[0]->root == root);
                    CHECK(items[1]->name == "TxtFile3");
                    CHECK(items[1]->parent->rid == folder);
                    CHECK(items[1]->root == root);
                }

                resourceTree.SortNodes(AssetTreeSort::Name, false);

                {
                    Span<AssetNode*> items = resourceTree.GetNode(folder)->nodes;
                    CHECK(items.Size() == 2);
                    CHECK(items[0]->name == "TxtFile3");
                    CHECK(items[1]->name == "TxtFile1");
                }
            }
        }
		Engine::Destroy();
	}

}