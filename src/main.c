//
// main.c
//
// z64scene entry point
// it all starts here
//

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "misc.h"
#include "test.h"
#include "project.h"
#include "logging.h"

extern void WindowMainLoop(const char *sceneFn);
extern void GuiTest(struct Project *project);

int main(int argc, char *argv[])
{
	struct Scene *scene = 0;
	
	ExePath(argv[0]);
	
	// tests
#ifndef NDEBUG
	// test wren
	TestWren();
	
	// test z64convert
	if (argc > 1)
	{
		char *which = argv[1];
		char *result = 0;
		
		if (!strcmp(which, "TestZ64convertScene"))
		{
			Testz64convertScene(&result);
		}
		else if (!strcmp(which, "TestZ64convertObject"))
		{
			char *objectPath = 0;
			Testz64convertObject(&objectPath);
			if (objectPath)
				LogDebug("probably worked, obj at '%s'", objectPath);
		}
		else if (!strcmp(which, "TestFast64toScene"))
		{
			if (!(which = argv[2])) Die("TestFast64toScene: not enough args");
			TestFast64toScene(which);
			return 0; // exit immediately after test
		}
		else if (!strcmp(which, "TestSaveLoadCycles"))
		{
			if (!(which = argv[2])) Die("TestSaveLoadCycles: not enough args");
			TestSaveLoadCycles(which);
			SceneWriterCleanup();
			return 0; // exit immediately after test
		}
		// test: opens and optionally exports a single scene (argv[2] -> argv[3])
		else if (!strcmp(which, "TestLoadSaveScene"))
		{
			if (!(which = argv[2]))
				Die("TestLoadSaveScene: not enough args: in.zscene [out.zscene]");
			scene = SceneFromFilenamePredictRooms(argv[2]);
			if (argc == 4)
				SceneToFilename(scene, argv[3]);
			SceneFree(scene);
			if (argc == 4)
				LogDebug("successfully wrote test scene");
			else
				LogDebug("successfully processed input scene");
			SceneWriterCleanup();
			return 0; // exit immediately after test
		}
		// test: tests the Swap() function
		else if (!strcmp(which, "TestSwapFunction"))
		{
			TestSwapFunction();
			return 0; // exit immediately after test
		}
		// test: tests scene visual/collision data migration functionality
		else if (!strcmp(which, "TestSceneMigrate"))
		{
			if (argc != 5)
				Die("not enough args: dst.zscene src.zscene out.zscene");
			TestSceneMigrate(argv[2], argv[3], argv[4]);
			return 0; // exit immediately after test
		}
		else if (!strcmp(which, "TestForEachActor"))
		{
			if (!(which = argv[2])) Die("TestForEachActor: not enough args");
			TestForEachActor(which);
			return 0; // exit immediately after test
		}
		else
			result = which;
		
		argv[1] = result;
	}
	
	// project loader test
	if (false)
	{
		struct Project *project = ProjectNewFromFilename(argv[1]);
		ExePath(argv[0]);
		
		FileListPrintAll(project->foldersObject);
		
		/*
		const char *objdir = FileListFindPathToId(project->foldersObject, 20);
		if (objdir)
		{
			sb_array(char *, files) = FileListFromDirectory(objdir, 1, true, false, false);
			FileListPrintAll(files);
			FileListFree(files);
		}
		*/
		
		GuiTest(project);
		
		ProjectFree(project);
		
		return 0;
	}
	
	// project directory -> file/folder list test
	if (false)
	{
		sb_array(char *, folders) = FileListFromDirectory(argv[1], 0, false, true, true);
		
		if (folders)
		{
			sb_array(char *, objects) = FileListFilterByWithVanilla(folders, "object", ".vanilla");
			
			FileListPrintAll(objects);
			
			if (sb_count(objects) > 0)
			{
				sb_array(char *, files) = FileListFromDirectory(objects[1], 1, true, false, false);
				FileListPrintAll(files);
				FileListFree(files);
			}
			
			FileListFree(objects);
			FileListFree(folders);
		}
		
		return 0;
	}

	// XXX: use the builtin test 'z64convert TestSaveLoadCycles path/to/scene' instead
	// test: open and re-export a single scene (argv[1] -> argv[2])
#if 0
	if (argc >= 2)
	{
		scene = SceneFromFilenamePredictRooms(argv[1]);
		if (argc == 3)
			SceneToFilename(scene, argv[2]);
		SceneFree(scene);
		if (argc == 3)
			LogDebug("successfully wrote test scene");
		else
			LogDebug("successfully processed input scene");
	}
	SceneWriterCleanup();
	return 0;
#endif

	// XXX: use the builtin test 'z64convert TestSaveLoadCycles path/to/project' instead
	// test: open and re-export a list of scenes
	// also does scene analysis
	// one filename per line, e.g.
	// bin/exhaustive/oot/scenes/0 - Dungeon/scene.zscene
	// bin/exhaustive/oot/scenes/1 - House/scene.zscene
	// bin/exhaustive/oot/scenes/2 - Town/scene.zscene
#if 0
	{
		//const char *fn = "bin/exhaustive-oot.txt";
		const char *fn = "bin/exhaustive-mm.txt";
		bool isAnalyzingScenes = false;
		// multiple passes to assert consistent use of rotations
		for (int i = 0; i < 1 + isAnalyzingScenes; ++i)
		{
			struct File *file = FileFromFilename(fn);
			char *src = file->data;
			char *tok;
			for (;;) {
				tok = strtok(src, "\r\n");
				if (!tok) break;
				src = 0;
				LogDebug("'%s'", tok);
				scene = SceneFromFilenamePredictRooms(tok);
				/*
				if (scene->rooms[0].headers[0].meshFormat == 1
					&& scene->rooms[0].headers[0].image.base.amountType
						== ROOM_SHAPE_IMAGE_AMOUNT_MULTI
				) Die("found ROOM_SHAPE_IMAGE_AMOUNT_MULTI");
				*/
				// want to analyze everything
				if (isAnalyzingScenes)
					TestAnalyzeSceneActors(scene, i < 1 ? 0 : "bin/TestAnalyzeSceneActors.log");
				// want to batch open/save everything
				else if (true)
					SceneToFilename(scene, 0);
				SceneFree(scene);
			}
			FileFree(file);
		}
		if (isAnalyzingScenes)
			TestAnalyzeSceneActors(0, 0);
		LogDebug("successfully wrote everything");
		SceneWriterCleanup();
		return 0;
	}
#endif
#endif // end tests
	
	ExePath(argv[0]);
	
	if (false && argc == 2)
	{
		scene = SceneFromFilenamePredictRooms(argv[1]);
		
		// test textures
		if (false)
		{
			struct File *file = FileFromFilename("bin/test.bin");
			
			DataBlobSegmentSetup(6, file->data, file->dataEnd, 0);
			
			DataBlobSegmentsPopulateFromMeshNew(0x06000000, 0);
			
			sb_array(struct TextureBlob, texBlobs) = 0;
			TextureBlobSbArrayFromDataBlobs(file, DataBlobSegmentGetHead(6), &texBlobs);
			scene->textureBlobs = texBlobs;
		}
	}
	else if (argc > 2)
	{
		struct File *file = FileFromFilename(argv[1]);
		struct DataBlob *blobs;
		uint32_t skeleton = 0;
		uint32_t animation = 0;
		
		for (int i = 2; i < argc; ++i)
		{
			const char *arg = argv[i];
			const char *next = argv[i + 1];
			
			while (*arg && !isalnum(*arg))
				++arg;
			
			if (!strcmp(arg, "skeleton"))
			{
				if (sscanf(next, "%x", &skeleton) != 1)
					Die("skeleton invalid argument no pointer '%s'", next);
				++i;
			}
			else if (!strcmp(arg, "animation"))
			{
				if (sscanf(next, "%x", &animation) != 1)
					Die("animation invalid argument no pointer '%s'", next);
				++i;
			}
		}
		
		if (!(skeleton && animation))
			Die("please provide a skeleton and an animation");
		
		// get texture blobs
		// very WIP, just want to view texture blobs for now
		blobs = MiscSkeletonDataBlobs(file, 0, skeleton);
		sb_array(struct TextureBlob, texBlobs) = 0;
		TextureBlobSbArrayFromDataBlobs(file, blobs, &texBlobs);
		scene = SceneFromFilenamePredictRooms("bin/scene.zscene");
		scene->textureBlobs = texBlobs;
	}
	
	WindowMainLoop(argv[1]);
	
	SceneWriterCleanup();
	return 0;
}

