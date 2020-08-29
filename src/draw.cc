#include <iostream>
#include <fstream>
#include <algorithm>

#include <nlohmann/json.hpp>

#include <TApplication.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TGraph2D.h>
#include <TH2.h>

#define STR1(x) #x
#define STR(x) STR1(x)

#define TEST(var) std::cout << \
  "\033[33m" STR(__LINE__) ": " \
  "\033[36m" #var ":\033[0m " << (var) << std::endl;

using std::cout;
using std::cerr;
using std::endl;
using nlohmann::json;

json opts;
template <typename T>
auto& opt(const auto&... k) {
  const json* x = &opts;
  ( ..., (x = &x->at(k)) );
  return *x->get_ptr<const T*>();
}

int main(int argc, char* argv[]) {
  if (argc<2) {
    cout << "usage: " << argv[0] << " in.json [name]\n";
    return 1;
  }

  const auto all_data = json::parse(std::ifstream(argv[1]));
  if (argc<3) {
    for (auto it : all_data.items())
      cout << it.key() << endl;
    return 0;
  }

  const auto& data = all_data.at(argv[2]);
  std::vector<double>
    fac  = data.at("fac"),
    ren  = data.at("ren"),
    xsec = data.at("xsec");

  int app_argc = 0;
  TApplication app("app", &app_argc, nullptr);

  opts = json::parse(std::ifstream("draw.json"));

  TCanvas canv;
  canv.SetLogx();
  canv.SetLogy();
  canv.SetTheta(opt<double>("canv","theta"));
  canv.SetPhi(opt<double>("canv","phi"));
  // canv.SetFrameLineColor(0);

  for (auto& x : xsec)
    if (x < 0.) x = 0.;

  TGraph2D g("","",xsec.size(),fac.data(),ren.data(),xsec.data());
  // gStyle->SetPalette(kBlueGreenYellow);
  // if (argc>3) {
  //   g.Draw(argv[3]); // tri2
  // } else {
  //   g.SetMarkerStyle(20);
  //   g.Draw("pcol");
  // }
  // TAxis* az = g.GetZaxis();

  const double max = *max_element(xsec.begin(),xsec.end());

  auto* h = g.GetHistogram();
  h->SetMinimum(0.);
  h->SetMaximum(max*1.05);

  g.Draw(opt<json::string_t>("g","draw").c_str());

  TGraph2D p(1);
  for (unsigned i=0; i<xsec.size(); ++i) {
    if (fac[i]==1 && ren[i]==1) {
      p.SetPoint(0,fac[i],ren[i],xsec[i]);
      break;
    }
  }
  p.Draw(opt<json::string_t>("p","draw").c_str());

  app.Run();
}
