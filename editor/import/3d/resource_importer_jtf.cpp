/**************************************************************************/
/*  resource_importer_jtf.cpp                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "resource_importer_jtf.h"

#include "core/error/error_macros.h"
#include "core/io/file_access.h"
#include "core/io/image_loader.h"
#include "core/io/resource_loader.h"
#include "core/math/math_defs.h"
#include "core/typedefs.h"
#include "core/variant/dictionary.h"
#include "core/variant/typed_array.h"
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/resources/3d/importer_mesh.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"
#include "scene/resources/surface_tool.h"

static void _attempt_to_load_texture(const String& p_basename, const String& p_suffix, Ref<StandardMaterial3D>& p_material, BaseMaterial3D::TextureParam p_param) {
	// Ignore already set parameter
	if (p_material->get_texture(p_param).is_valid()) {
		return;
	}

	List<String> extensions;
	ImageLoader::get_recognized_extensions(&extensions);
	for (const String& ext : extensions) {
		String texture_path = p_basename + p_suffix + "." + ext;
		if (ResourceLoader::exists(texture_path)) {
			Ref<Texture2D> texture = ResourceLoader::load(texture_path);
			if (texture.is_valid()) {
				p_material->set_texture(p_param, texture);
				break;
			}
		}
	}
}

static Error _parse_jtf(const String &p_path, Ref<ImporterMesh> &r_mesh, bool p_generate_tangents, bool p_generate_lods, bool p_generate_shadow_mesh, bool p_generate_lightmap_uv2, float p_generate_lightmap_uv2_texel_size, bool p_disable_compression) {
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_CANT_OPEN, vformat("Couldn't open JTF file '%s', it may not exist or not be readable.", p_path));

	// Confirm magic numbers
	uint8_t magic0 = f->get_8();
	uint8_t magic1 = f->get_8();
	uint8_t magic2 = f->get_8();
	uint8_t magic3 = f->get_8();
	ERR_FAIL_COND_V_MSG(magic0 != 74 || magic1 != 84 || magic2 != 70 || magic3 != 33, ERR_FILE_CORRUPT, vformat("Couldn't read JTF file '%s', the magic numbers are wrong.", p_path));

	// Confirm version
	uint32_t version = f->get_32();
	ERR_FAIL_COND_V_MSG(version != 0, ERR_FILE_CORRUPT, vformat("Couldn't read JTF file '%s', version %i is unsupported.", p_path, (int)version));

	// Read face count
	uint32_t face_count = f->get_32();

	// Read mesh data
	Vector<float> mesh_data;
	Ref<SurfaceTool> surf_tool;
	surf_tool.instantiate();
	surf_tool->begin(Mesh::PRIMITIVE_TRIANGLES);
	for (uint32_t face_index = 0; face_index < face_count; ++face_index) {
		real_t x0 = f->get_float();
		real_t y0 = f->get_float();
		real_t z0 = f->get_float();
		real_t nx0 = f->get_float();
		real_t ny0 = f->get_float();
		real_t nz0 = f->get_float();
		real_t s0 = f->get_float();
		real_t t0 = f->get_float();
		real_t x1 = f->get_float();
		real_t y1 = f->get_float();
		real_t z1 = f->get_float();
		real_t nx1 = f->get_float();
		real_t ny1 = f->get_float();
		real_t nz1 = f->get_float();
		real_t s1 = f->get_float();
		real_t t1 = f->get_float();
		real_t x2 = f->get_float();
		real_t y2 = f->get_float();
		real_t z2 = f->get_float();
		real_t nx2 = f->get_float();
		real_t ny2 = f->get_float();
		real_t nz2 = f->get_float();
		real_t s2 = f->get_float();
		real_t t2 = f->get_float();
		surf_tool->set_uv(Vector2(s0, t0));
		surf_tool->set_normal(Vector3(nx0, ny0, nz0));
		surf_tool->add_vertex(Vector3(x0, y0, z0));
		surf_tool->set_uv(Vector2(s2, t2));
		surf_tool->set_normal(Vector3(nx2, ny2, nz2));
		surf_tool->add_vertex(Vector3(x2, y2, z2));
		surf_tool->set_uv(Vector2(s1, t1));
		surf_tool->set_normal(Vector3(nx1, ny1, nz1));
		surf_tool->add_vertex(Vector3(x1, y1, z1));
	}


	// Extract name
	String basename = p_path.get_basename();
	String name;
	int slash_pos = MAX(basename.rfind_char('/'), basename.rfind_char('\\'));
	if (slash_pos == -1) {
		name = basename;
	} else {
		name = basename.substr(slash_pos + 1);
	}

	// Generate material
	Ref<StandardMaterial3D> material;
	material.instantiate();
	material->set_name(name);
	material->set_specular(0.0f);
	_attempt_to_load_texture(basename, "", material, BaseMaterial3D::TextureParam::TEXTURE_ALBEDO);
	_attempt_to_load_texture(basename, "_a", material, BaseMaterial3D::TextureParam::TEXTURE_ALBEDO);
	_attempt_to_load_texture(basename, "_n", material, BaseMaterial3D::TextureParam::TEXTURE_NORMAL);
	_attempt_to_load_texture(basename, "_r", material, BaseMaterial3D::TextureParam::TEXTURE_ROUGHNESS);
	_attempt_to_load_texture(basename, "_ao", material, BaseMaterial3D::TextureParam::TEXTURE_AMBIENT_OCCLUSION);
	_attempt_to_load_texture(basename, "_e", material, BaseMaterial3D::TextureParam::TEXTURE_EMISSION);

	// Postprocess mesh
	if (p_generate_tangents) {
		surf_tool->generate_tangents();
	}
	surf_tool->index();
	surf_tool->set_material(material);

	// Generate mesh
	r_mesh.instantiate();
	Array array = surf_tool->commit_to_arrays();
	r_mesh->add_surface(Mesh::PRIMITIVE_TRIANGLES, array, TypedArray<Array>(), Dictionary(), material, name, RS::ARRAY_FLAG_COMPRESS_ATTRIBUTES);

	if (p_generate_lightmap_uv2) {
		Vector<uint8_t> lightmap_cache;
		r_mesh->lightmap_unwrap_cached(Transform3D(), p_generate_lightmap_uv2_texel_size, Vector<uint8_t>(), lightmap_cache);
	}
	if (p_generate_lods) {
		r_mesh->generate_lods(60.0f, {});
	}
	if (p_generate_shadow_mesh) {
		r_mesh->create_shadow_mesh();
	}
	r_mesh->optimize_indices();

	return OK;
}

Node *EditorJTFImporter::import_scene(const String &p_path, uint32_t p_flags, const HashMap<StringName, Variant> &p_options, List<String> *r_missing_deps, Error *r_err) {
	Ref<ImporterMesh> mesh;

	Error err = _parse_jtf(p_path, mesh, p_flags & IMPORT_GENERATE_TANGENT_ARRAYS, false, false, false, 0.2, p_flags & IMPORT_FORCE_DISABLE_MESH_COMPRESSION);

	if (err != OK) {
		if (r_err) {
			*r_err = err;
		}
		return nullptr;
	}

	Node3D *scene = memnew(Node3D);


	ImporterMeshInstance3D *mi = memnew(ImporterMeshInstance3D);
	mi->set_mesh(mesh);
	mi->set_name(mesh->get_name());
	scene->add_child(mi, true);
	mi->set_owner(scene);

	if (r_err) {
		*r_err = OK;
	}

	return scene;
}

void EditorJTFImporter::get_extensions(List<String> *r_extensions) const {
	r_extensions->push_back("jtf");
}

EditorJTFImporter::EditorJTFImporter() {
}

////////////////////////////////////////////////////

String ResourceImporterJTF::get_importer_name() const {
	return "just_triangle_faces";
}

String ResourceImporterJTF::get_visible_name() const {
	return "JTF as Mesh";
}

void ResourceImporterJTF::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("jtf");
}

String ResourceImporterJTF::get_save_extension() const {
	return "mesh";
}

String ResourceImporterJTF::get_resource_type() const {
	return "Mesh";
}

int ResourceImporterJTF::get_format_version() const {
	return 1;
}

int ResourceImporterJTF::get_preset_count() const {
	return 0;
}

String ResourceImporterJTF::get_preset_name(int p_idx) const {
	return "";
}

void ResourceImporterJTF::get_import_options(const String &p_path, List<ImportOption> *r_options, int p_preset) const {
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "generate_tangents"), true));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "generate_lods"), true));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "generate_shadow_mesh"), true));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "generate_lightmap_uv2", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED), false));
	r_options->push_back(ImportOption(PropertyInfo(Variant::FLOAT, "generate_lightmap_uv2_texel_size", PROPERTY_HINT_RANGE, "0.001,100,0.001"), 0.2));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "force_disable_mesh_compression"), false));
}

bool ResourceImporterJTF::get_option_visibility(const String &p_path, const String &p_option, const HashMap<StringName, Variant> &p_options) const {
	if (p_option == "generate_lightmap_uv2_texel_size" && !p_options["generate_lightmap_uv2"]) {
		// Only display the lightmap texel size import option when lightmap UV2 generation is enabled.
		return false;
	}

	return true;
}

Error ResourceImporterJTF::import(ResourceUID::ID p_source_id, const String &p_source_file, const String &p_save_path, const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
	Ref<ImporterMesh> mesh;

	Error err;

	err = _parse_jtf(p_source_file, mesh, p_options["generate_tangents"], p_options["generate_lods"], p_options["generate_shadow_mesh"], p_options["generate_lightmap_uv2"], p_options["generate_lightmap_uv2_texel_size"], p_options["force_disable_mesh_compression"]);
	ERR_FAIL_COND_V(err != OK, err);

	String save_path = p_save_path + ".mesh";

	err = ResourceSaver::save(mesh->get_mesh(), save_path);

	ERR_FAIL_COND_V_MSG(err != OK, err, "Cannot save Mesh to file '" + save_path + "'.");

	r_gen_files->push_back(save_path);

	return OK;
}

ResourceImporterJTF::ResourceImporterJTF() {
}
