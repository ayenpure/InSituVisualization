#include <ascent.hpp>
#include <assert.h>
#include <cmath>
#include <conduit.hpp>
#include <conduit_blueprint.hpp>
#include <iostream>
#include <sstream>
#include <string.h>

#define NX 50
#define NY 50
#define NZ 50
#define PI 3.14159265
#define PERIOD 100

double calculateVelocityMagnitude(double x, double y, double z, double t) {
  return sin(PI * sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2)) + t);
}

struct Options {
  int m_dims[3];
  double m_spacing[3];
  int m_time_steps;
  double m_time_delta;
  Options() : m_dims{NX, NY, NZ}, m_time_steps(10), m_time_delta(0.5) {
    SetSpacing();
  }
  void SetSpacing() {
    m_spacing[0] = 10. / double(m_dims[0]);
    m_spacing[1] = 10. / double(m_dims[1]);
    m_spacing[2] = 10. / double(m_dims[2]);
  }
  void Parse(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
      if (contains(argv[i], "--dims=")) {
        std::string s_dims;
        s_dims = GetArg(argv[i]);
        std::vector<std::string> dims;
        dims = split(s_dims, ',');

        if (dims.size() != 3) {
          Usage(argv[i]);
        }

        m_dims[0] = stoi(dims[0]);
        m_dims[1] = stoi(dims[1]);
        m_dims[2] = stoi(dims[2]);
        SetSpacing();
      } else if (contains(argv[i], "--time_steps=")) {

        std::string time_steps;
        time_steps = GetArg(argv[i]);
        m_time_steps = stoi(time_steps);
      } else if (contains(argv[i], "--time_delta=")) {

        std::string time_delta;
        time_delta = GetArg(argv[i]);
        m_time_delta = stof(time_delta);
      } else {
        Usage(argv[i]);
      }
    }
  }

  std::string GetArg(const char *arg) {
    std::vector<std::string> parse;
    std::string s_arg(arg);
    std::string res;

    parse = split(s_arg, '=');

    if (parse.size() != 2) {
      Usage(arg);
    } else {
      res = parse[1];
    }
    return res;
  }
  void Print() const {
    std::cout << "======== Noise Options =========\n";
    std::cout << "dims       : (" << m_dims[0] << ", " << m_dims[1] << ", "
              << m_dims[2] << ")\n";
    std::cout << "spacing    : (" << m_spacing[0] << ", " << m_spacing[1]
              << ", " << m_spacing[2] << ")\n";
    std::cout << "time steps : " << m_time_steps << "\n";
    std::cout << "time delta : " << m_time_delta << "\n";
    std::cout << "================================\n";
  }

  void Usage(std::string bad_arg) {
    std::cerr << "Invalid argument \"" << bad_arg << "\"\n";
    std::cout
        << "Noise usage: "
        << "       --dims       : global data set dimensions (ex: "
           "--dims=32,32,32)\n"
        << "       --time_steps : number of time steps  (ex: --time_steps=10)\n"
        << "       --time_delta : amount of time to advance per time step  "
           "(ex: --time_delta=0.5)\n";
    exit(0);
  }

  std::vector<std::string> &split(const std::string &s, char delim,
                                  std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;

    while (std::getline(ss, item, delim)) {
      elems.push_back(item);
    }
    return elems;
  }

  std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
  }

  bool contains(const std::string haystack, std::string needle) {
    std::size_t found = haystack.find(needle);
    return (found != std::string::npos);
  }
};

struct SpatialDivision {
  int m_mins[3];
  int m_maxs[3];

  SpatialDivision() : m_mins{0, 0, 0}, m_maxs{1, 1, 1} {}

  bool CanSplit(int dim) { return m_maxs[dim] - m_mins[dim] + 1 > 1; }

  SpatialDivision Split(int dim) {
    SpatialDivision r_split;
    r_split = *this;
    assert(CanSplit(dim));
    int size = m_maxs[dim] - m_mins[dim] + 1;
    int left_offset = size / 2;

    // shrink the left side
    m_maxs[dim] = m_mins[dim] + left_offset - 1;
    // shrink the right side
    r_split.m_mins[dim] = m_maxs[dim] + 1;
    return r_split;
  }
};

struct DataSet {
  const int m_cell_dims[3];
  const int m_point_dims[3];
  const int m_cell_size;
  const int m_point_size;
  double *m_nodal_scalars;
  double *m_zonal_scalars;
  double m_spacing[3];
  double m_origin[3];
  double m_time_step;

  DataSet(const Options &options, const SpatialDivision &div)
      : m_cell_dims{div.m_maxs[0] - div.m_mins[0] + 1,
                    div.m_maxs[1] - div.m_mins[1] + 1,
                    div.m_maxs[2] - div.m_mins[2] + 1},
        m_point_dims{m_cell_dims[0] + 1, m_cell_dims[1] + 1,
                     m_cell_dims[2] + 1},
        m_cell_size(m_cell_dims[0] * m_cell_dims[1] * m_cell_dims[2]),
        m_point_size(m_point_dims[0] * m_point_dims[1] * m_point_dims[2]),
        m_spacing{options.m_spacing[0], options.m_spacing[1],
                  options.m_spacing[2]},
        m_origin{0. + double(div.m_mins[0]) * m_spacing[0],
                 0. + double(div.m_mins[1]) * m_spacing[1],
                 0. + double(div.m_mins[2]) * m_spacing[2]}

  {
    m_nodal_scalars = new double[m_point_size];
    m_zonal_scalars = new double[m_cell_size];
  }

  inline void GetCoord(const int &x, const int &y, const int &z,
                       double *coord) {
    coord[0] = m_origin[0] + m_spacing[0] * double(x);
    coord[1] = m_origin[1] + m_spacing[1] * double(y);
    coord[2] = m_origin[2] + m_spacing[2] * double(z);
  }
  inline void SetPoint(const double &val, const int &x, const int &y,
                       const int &z) {
    const int offset =
        z * m_point_dims[0] * m_point_dims[1] + y * m_point_dims[0] + x;
    m_nodal_scalars[offset] = val;
  }

  inline void SetCell(const double &val, const int &x, const int &y,
                      const int &z) {
    const int offset =
        z * m_cell_dims[0] * m_cell_dims[1] + y * m_cell_dims[0] + x;
    m_zonal_scalars[offset] = val;
  }

  void PopulateNode(conduit::Node &node) {
    node["coordsets/coords/type"] = "uniform";

    node["coordsets/coords/dims/i"] = m_point_dims[0];
    node["coordsets/coords/dims/j"] = m_point_dims[1];
    node["coordsets/coords/dims/k"] = m_point_dims[2];

    node["coordsets/coords/origin/x"] = m_origin[0];
    node["coordsets/coords/origin/y"] = m_origin[1];
    node["coordsets/coords/origin/z"] = m_origin[2];

    node["coordsets/coords/spacing/dx"] = m_spacing[0];
    node["coordsets/coords/spacing/dy"] = m_spacing[1];
    node["coordsets/coords/spacing/dz"] = m_spacing[2];

    node["topologies/mesh/type"] = "uniform";
    node["topologies/mesh/coordset"] = "coords";

    node["fields/nodal_noise/association"] = "vertex";
    node["fields/nodal_noise/type"] = "scalar";
    node["fields/nodal_noise/topology"] = "mesh";
    node["fields/nodal_noise/values"].set_external(m_nodal_scalars);

    node["fields/zonal_noise/association"] = "element";
    node["fields/zonal_noise/type"] = "scalar";
    node["fields/zonal_noise/topology"] = "mesh";
    node["fields/zonal_noise/values"].set_external(m_zonal_scalars);
  }

  void Print() {
    std::cout << "Origin "
              << "(" << m_origin[0] << " -  "
              << m_origin[0] + m_spacing[0] * m_cell_dims[0] << "), "
              << "(" << m_origin[1] << " -  "
              << m_origin[1] + m_spacing[1] * m_cell_dims[1] << "), "
              << "(" << m_origin[2] << " -  "
              << m_origin[2] + m_spacing[2] * m_cell_dims[2] << ")\n ";
  }

  ~DataSet() {
    if (m_nodal_scalars)
      delete[] m_nodal_scalars;
    if (m_zonal_scalars)
      delete[] m_zonal_scalars;
  }

private:
  DataSet()
      : m_cell_dims{1, 1, 1}, m_point_dims{2, 2, 2}, m_cell_size(1),
        m_point_size(8) {
    m_nodal_scalars = NULL;
    m_zonal_scalars = NULL;
  };
};

void Init(SpatialDivision &div, const Options &options) { options.Print(); }

int main(int argc, char **argv) {
  std::cout << "We be simulatin' all day!!!" << std::endl;

  Options options;
  options.Parse(argc, argv);

  SpatialDivision div;
  //
  // Inclusive range. Ex cell dim = 32
  // then the div is [0,31]
  //
  div.m_maxs[0] = options.m_dims[0] - 1;
  div.m_maxs[1] = options.m_dims[1] - 1;
  div.m_maxs[2] = options.m_dims[2] - 1;

  Init(div, options);
  DataSet data_set(options, div);

  double spatial_extents[3];
  spatial_extents[0] = options.m_spacing[0] * options.m_dims[0] + 1;
  spatial_extents[1] = options.m_spacing[1] * options.m_dims[1] + 1;
  spatial_extents[2] = options.m_spacing[2] * options.m_dims[2] + 1;

  double time = 0;
  //
  //  Open and setup ascent
  //
  ascent::Ascent ascent;
  conduit::Node ascent_opts;
  ascent_opts["runtime/type"] = "ascent";
  ascent.open(ascent_opts);

  conduit::Node mesh_data;
  mesh_data["state/time"].set_external(&time);
  mesh_data["state/cycle"].set_external(&time);
  mesh_data["state/domain_id"] = 0;
  mesh_data["state/info"] = "Pseudocolor of random math function";
  data_set.PopulateNode(mesh_data);

  /*conduit::Node pipelines;
  // pipeline 1
  pipelines["pl1/f1/type"] = "contour";
  // filter knobs
  conduit::Node &contour_params = pipelines["pl1/f1/params"];
  contour_params["field"] = "nodal_noise";
  contour_params["iso_values"] = 0.4;*/

  conduit::Node scenes;
  scenes["scene1/plots/plt1/type"] = "pseudocolor";
  //scenes["scene1/plots/plt1/pipeline"] = "pl1";
  scenes["scene1/plots/plt1/params/field"] = "nodal_noise";

  conduit::Node actions;
  /*conduit::Node &add_pipelines = actions.append();
  add_pipelines["action"] = "add_pipelines";
  add_pipelines["pipelines"] = pipelines;*/

  conduit::Node &add_scenes = actions.append();
  add_scenes["action"] = "add_scenes";
  add_scenes["scenes"] = scenes;

  conduit::Node &execute = actions.append();
  execute["action"] = "execute";

  conduit::Node reset;
  conduit::Node &reset_action = reset.append();
  reset_action["action"] = "reset";

  for (int t = 0; t < options.m_time_steps; ++t) {
    //
    // update scalars
    //
    for (int z = 0; z < data_set.m_point_dims[2]; ++z)
      for (int y = 0; y < data_set.m_point_dims[1]; ++y)
        for (int x = 0; x < data_set.m_point_dims[0]; ++x) {
          double val_point = calculateVelocityMagnitude(x, y, z, time);
          data_set.SetPoint(val_point, x, y, z);
        }
    time += options.m_time_delta;
    ascent.publish(mesh_data);
    ascent.execute(actions);
    ascent.execute(reset);
  } // for each time step

  ascent.close();
  return 0;
}
