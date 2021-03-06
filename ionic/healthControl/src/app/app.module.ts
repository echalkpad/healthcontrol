import { NgModule } from '@angular/core';
import { IonicApp, IonicModule } from 'ionic-angular';
import { MyApp } from './app.component';
import { HomePage } from '../pages/home/home';
import { Medico } from '../pages/medico/medico';
import { Configuracao } from '../pages/configuracao/configuracao';
import { Historico } from '../pages/historico/historico';
import { Login } from '../pages/login/login';
import { HistoricoFilho } from '../pages/historico-filho/historico-filho';
@NgModule({
  declarations: [
    MyApp,
    HomePage,
    Medico,
    Login,
    Configuracao,
    Historico,
    HistoricoFilho
  ],
  imports: [
    IonicModule.forRoot(MyApp)
  ],
  bootstrap: [IonicApp],
  entryComponents: [
    MyApp,
    HomePage,
    Medico,
    Login,
    Configuracao,
    Historico,
    HistoricoFilho
  ],
  providers: []
})
export class AppModule {}
