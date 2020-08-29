#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <optional>
#include <memory>
#include <algorithm>
#include <numeric>

#include <TFile.h>
#include <TKey.h>
#include <TChain.h>
#include <TEnv.h>

#include <fastjet/ClusterSequence.hh>

#define STR1(x) #x
#define STR(x) STR1(x)

#define TEST(var) std::cout << \
  "\033[33m" STR(__LINE__) ": " \
  "\033[36m" #var ":\033[0m " << (var) << std::endl;

#include "ivanp/error.hh"
#include "ivanp/enumerate.hh"
#include "ivanp/timed_counter.hh"
#include "ivanp/branch_reader.hh"
#include "ivanp/vec4.hh"
#include "ivanp/hist/histograms.hh"
#include "ivanp/hist/bins.hh"
#include "ivanp/hist/json.hh"

#include "reweighter.hh"
#include "reweighter_json.hh"
#include "Higgs2diphoton.hh"

using namespace ivanp::cont::ops::map;

template <ivanp::cont::Container C>
decltype(auto) operator+=(std::vector<auto,auto>& v, C&& r) {
  v.reserve(v.size()+ivanp::cont::size(r));
  std::forward<C>(r) | [&]<typename T>(T&& x){
    v.emplace_back(std::forward<T>(x));
  };
  return v;
}

using std::cout;
using std::cerr;
using std::endl;
using nlohmann::json;
namespace fj = fastjet;
using namespace ivanp;

struct single_bin_axis {
  using edge_type = int;
  using index_type = ivanp::hist::index_type;
  index_type nbins() const noexcept { return 1; }
  index_type find_bin_index(edge_type x) const noexcept { return 0; }
};
void to_json(nlohmann::json& j, const single_bin_axis& axis) noexcept {
  j = nullptr;
}

using bin_t = ivanp::hist::nlo_mc_multibin;
using hist_t = ivanp::hist::histogram<
  bin_t,
  ivanp::hist::axes_spec<std::array<single_bin_axis,1>>,
  ivanp::hist::bins_spec<std::array<ivanp::hist::nlo_mc_multibin,1>>
>;

std::vector<std::string> weights_names;
template <>
struct ivanp::hist::bin_def<bin_t> {
  static nlohmann::json def() {
    return { weights_names, "n", "nent" };
  }
};

bool photon_eta_cut(double abs_eta) noexcept {
  return (1.37 < abs_eta && abs_eta < 1.52) || (2.37 < abs_eta);
}

int main(int argc, char* argv[]) {
  if (argc < 4) {
    cerr << "usage: " << argv[0] <<
      " config.json hists.json ntuple1.root ... \n";
    return 1;
  }

  const auto conf = json::parse(std::ifstream(argv[1]));
  cout << conf/*.dump(2)*/ <<'\n'<< endl;

  // Chain input files
  std::unique_ptr<TChain> chain;
  { auto file = std::make_unique<TFile>(argv[3]);
    TTree* tree = nullptr;
    for (auto* _key : *file->GetListOfKeys()) { // find TTree
      auto* key = static_cast<TKey*>(_key);
      const auto* key_class = TClass::GetClass(key->GetClassName(),true);
      if (!key_class) continue;
      if (key_class->InheritsFrom(TTree::Class())) {
        if (!tree)
          tree = dynamic_cast<TTree*>(key->ReadObj());
        else THROW("multiple trees in file \"",file->GetName(),"\"");
      }
    }
    chain = std::make_unique<TChain>(tree->GetName());
    cout << "Tree name: " << chain->GetName() << '\n';
  }
  for (int i=3; i<argc; ++i) {
    cout << argv[i] << endl;
    if (!chain->Add(argv[i],0)) THROW("failed to add file to chain");
  }
  cout << endl;

  // Read branches
  TTreeReader reader(&*chain);
  branch_reader<int>
    _id(reader,"id"),
    _nparticle(reader,"nparticle");
  branch_reader<double[],float[]>
    _px(reader,"px"),
    _py(reader,"py"),
    _pz(reader,"pz"),
    _E (reader,"E" );
  branch_reader<int[]> _kf(reader,"kf");
  branch_reader<double> _weight2(reader,"weight2");

  std::optional<branch_reader<int>> _ncount;
  for (auto* b : *reader.GetTree()->GetListOfBranches()) {
    if (!strcmp(b->GetName(),"ncount")) {
      _ncount.emplace(reader,"ncount");
      break;
    }
  }

  weights_names = { "weight2" };

  // Make reweighters
  auto reweighters = conf.at("reweighting") | [&](auto def){
    auto& ren_fac = def["ren_fac"];
    const int step_div = ren_fac[1];
    const int max = int(ren_fac[0])*step_div;
    const double step = 1./step_div;
    std::vector<std::array<double,2>> scales;
    scales.reserve(sq(max*2+1));
    for (int r = -max; r<=max; ++r)
      for (int f = -max; f<=max; ++f)
        scales.push_back({ std::exp2(r*step), std::exp2(f*step) });
    ren_fac = scales;

    reweighter rew(reader,def);
    weights_names += rew.weights_names();
    return rew;
  };
  cout << endl;

  // weight vector needs to be resized before histograms are created
  bin_t::weight.resize( weights_names.size() );
  for (const auto& name : weights_names)
    cout << name << '\n';
  cout << endl;

  // create histograms ----------------------------------------------
  // hist_t h_total(hist_t::axes_type{});
  hist_t h_total;

  std::map<const char*,hist_t*,chars_less> hists {
    {"total",&h_total}
  };

  std::vector<fj::PseudoJet> partons;
  std::vector<vec4> jets;
  Higgs2diphoton higgs_decay(
    conf.value("higgs_decay_seed",Higgs2diphoton::seed_type(0)));
  Higgs2diphoton::photons_type photons;
  vec4 higgs;

  const fj::JetDefinition jet_def(
    fj::antikt_algorithm, conf.value("jet_R",0.4) );
  fj::ClusterSequence::print_banner(); // get it out of the way
  cout << jet_def.description() << endl;

  const double
    jet_pt_cut = conf.value("jet_pt_cut",30.),
    jet_eta_cut = conf.value("jet_eta_cut",4.4);
  const unsigned njets_min = conf.value("njets_min",0u);
  const bool apply_photon_cuts = conf.value("apply_photon_cuts",false);

  cout << endl;

  bin_t::id = -1; // so that first entry has new id
  long unsigned Ncount = 0, Nevents = 0, Nentries = chain->GetEntries();

  // EVENT LOOP =====================================================
  for (timed_counter cnt(Nentries); reader.Next(); ++cnt) {
    const bool new_id = [id=*_id]{
      return (bin_t::id != id) ? ((bin_t::id = id),true) : false;
    }();
    if (new_id) {
      Ncount += (_ncount ? **_ncount : 1);
      ++Nevents;
    }

    // read 4-momenta -----------------------------------------------
    partons.clear();
    const unsigned np = *_nparticle;
    bool got_higgs = false;
    for (unsigned i=0; i<np; ++i) {
      if (_kf[i] == 25) {
        higgs = { _px[i],_py[i],_pz[i],_E[i] };
        got_higgs = true;
      } else {
        partons.emplace_back(_px[i],_py[i],_pz[i],_E[i]);
      }
    }
    if (!got_higgs) THROW("event without Higgs boson (kf==25)");

    // H -> γγ ----------------------------------------------------
    if (apply_photon_cuts) {
      photons = higgs_decay(higgs,new_id);
      auto A_pT = photons | [](const auto& p){ return p.pt(); };
      if (A_pT[0] < A_pT[1]) {
        std::swap(A_pT[0],A_pT[1]);
        std::swap(photons[0],photons[1]);
      }
      const auto A_eta = photons | [](const auto& p){ return p.eta(); };

      if (
        (A_pT[0] < 0.35*125.) or
        (A_pT[1] < 0.25*125.) or
        photon_eta_cut(std::abs(A_eta[0])) or
        photon_eta_cut(std::abs(A_eta[1]))
      ) continue;
    }

    // Jets ---------------------------------------------------------
    jets = fj::ClusterSequence(partons,jet_def)
          .inclusive_jets() // get clustered jets
          | [](const auto& j){ return vec4(j); };

    jets.erase( std::remove_if( jets.begin(), jets.end(), // apply jet cuts
      [=](const auto& jet){
        return (jet.pt() < jet_pt_cut)
        or (std::abs(jet.eta()) > jet_eta_cut);
      }), jets.end() );
    std::sort( jets.begin(), jets.end(), // sort by pT
      [](const auto& a, const auto& b){ return ( a.pt() > b.pt() ); });
    const unsigned njets = jets.size(); // number of clustered jets

    // set weights --------------------------------------------------
    { auto w = bin_t::weight.begin();
      *w = *_weight2;
      for (auto& rew : reweighters) {
        rew(); // reweight this event
        for (unsigned i=0, n=rew.nweights(); i<n; ++i)
          *++w = rew[i];
      }
    }

    // Observables **************************************************
    if (njets < njets_min) continue; // require minimum number of jets

    h_total(0);
  } // end event loop
  // ================================================================
  cout << endl;

  // finalize bins
  for (auto& [name,h] : hists)
    for (auto& bin : *h)
      bin.finalize();

  // write output file
  { std::ofstream out(argv[2]);
    json jhists(hists);
    jhists["N"] = {
      {"entries",Nentries},
      {"events",Nevents},
      {"count",Ncount}
    };
    if (ends_with(argv[2],".cbor")) {
      auto cbor = json::to_cbor(jhists);
      out.write(
        reinterpret_cast<const char*>(cbor.data()),
        cbor.size() * sizeof(decltype(cbor[0]))
      );
    } else {
      out << jhists << '\n';
    }
  }
}
