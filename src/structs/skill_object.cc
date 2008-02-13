#include <skill_object.h>

map<int, SkillObject *> SkillObject::skillMap;

SkillObject::~SkillObject() {
}

void
SkillObject::perform(Creature *ch, Creature *target, ExecutableObject *eo) 
{
    eo->setChar(ch);

    if (eo->isA(ESkillAffect)) {
        SkillAffect *sa = dynamic_cast<SkillAffect *>(eo);
        sa->setSkillNum(_skillNum);
    }
  
    eo->setVictim(target);
}

void
SkillObject::perform(Creature *ch, obj_data *target, ExecutableObject *eo) 
{
    eo->setChar(ch);

    if (eo->isA(ESkillAffect)) {
        SkillAffect *sa = dynamic_cast<SkillAffect *>(eo);
        sa->setSkillNum(_skillNum);
    }
  
    eo->setObject(target);
}

void
SkillObject::perform(Creature *ch, CreatureList &targets, ExecutableVector &ev)
{
    CreatureList::iterator it = targets.begin();
    for (; it != targets.end(); ++it) {
        this->perform(ch, *it, ev);
    }
}

void 
SkillObject::defend(ExecutableVector &ev)
{
    ExecutableVector::iterator vi;
    for (vi = ev.begin(); vi != ev.end(); ++vi) {
        bool save = mag_savingthrow((*vi)->getVictim(), 
                                    (*vi)->getChar()->getLevelBonus(_skillNum) /
                                    2, _saveType);
        if ((*vi)->isA(ESkillAffect)) {
            SkillAffect *sa = dynamic_cast<SkillAffect *>(*vi);
            if (sa->getSkillNum() == _skillNum) {
                if (save) {
                   ev.erase(vi); 
                   vi = ev.begin();
                }
            }
        }
        else if ((*vi)->isA(EDamageObject)) {
            DamageObject *do1 = dynamic_cast<DamageObject *>(*vi);
            if (do1->getDamageType() == _skillNum) {
                if (save) {
                    ev.erase(vi);
                    vi = ev.begin();
                }
            }
        }
    }
}

// Make sure you add your new spells and skills thusly
void
mag_assign_spells(void) {
    new SpellAirWalk();
    new SpellArmor();
    new SpellAstralSpell();
    new SpellControlUndead();
    new SpellTeleport();
    new SpellLocalTeleport();
    new SpellBlur();
    new SpellBless();
    new SpellDamn();
    new SpellCalm();
    new SpellBlindness();
    new SpellBreatheWater();
    new SpellBurningHands();
    new SpellCallLightning();
    new SpellEnvenom();
    new SpellCharm();
    new SpellCharmAnimal();
    new SpellChillTouch();
    new SpellClairvoyance();
    new SpellCallRodent();
    new SpellCallBird();
    new SpellCallReptile();
    new SpellCallBeast();
    new SpellCallPredator();
    new SpellClone();
    new SpellColorSpray();
    new SpellCommand();
    new SpellConeCold();
    new SpellConjureElemental();
    new SpellControlWeather();
    new SpellCreateFood();
    new SpellCreateWater();
    new SpellCureBlind();
    new SpellCureCritic();
    new SpellCureLight();
    new SpellCurse();
    new SpellDetectAlign();
    new SpellDetectInvis();
    new SpellDetectMagic();
    new SpellDetectPoison();
    new SpellDetectScrying();
    new SpellDimensionDoor();
    new SpellDispelEvil();
    new SpellDispelGood();
    new SpellDispelMagic();
    new SpellDisruption();
    new SpellDisplacement();
    new SpellEarthquake();
    new SpellEnchantArmor();
    new SpellEnchantWeapon();
    new SpellEndureCold();
    new SpellEnergyDrain();
    new SpellFly();
    new SpellFlameStrike();
    new SpellFlameOfFaith();
    new SpellGoodberry();
    new SpellGustOfWind();
    new SpellBarkskin();
    new SpellIcyBlast();
    new SpellInvisToUndead();
    new SpellAnimalKin();
    new SpellGreaterEnchant();
    new SpellGreaterInvis();
    new SpellGroupArmor();
    new SpellFireball();
    new SpellFireShield();
    new SpellGreaterHeal();
    new SpellGroupHeal();
    new SpellHarm();
    new SpellHeal();
    new SpellDivineIllumination();
    new SpellHaste();
    new SpellInfravision();
    new SpellInvisible();
    new SpellGlowlight();
    new SpellKnock();
    new SpellLightningBolt();
    new SpellLocateObject();
    new SpellMagicMissile();
    new SpellMinorIdentify();
    new SpellMagicalProt();
    new SpellMagicalVestment();
    new SpellMeteorStorm();
    new SpellChainLightning();
    new SpellHailstorm();
    new SpellIceStorm();
    new SpellPoison();
    new SpellPray();
    new SpellPrismaticSpray();
    new SpellProtectFromDevils();
    new SpellProtFromEvil();
    new SpellProtFromGood();
    new SpellProtFromLightning();
    new SpellProtFromFire();
    new SpellRemoveCurse();
    new SpellRemoveSickness();
    new SpellRejuvenate();
    new SpellRefresh();
    new SpellRegenerate();
    new SpellRetrieveCorpse();
    new SpellSanctuary();
    new SpellShockingGrasp();
    new SpellShroudObscurement();
    new SpellSleep();
    new SpellSlow();
    new SpellSpiritHammer();
    new SpellStrength();
    new SpellSummon();
    new SpellSword();
    new SpellSymbolOfPain();
    new SpellStoneToFlesh();
    new SpellTelekinesis();
    new SpellWordStun();
    new SpellTrueSeeing();
    new SpellWordOfRecall();
    new SpellWordOfIntellect();
    new SpellPeer();
    new SpellRemovePoison();
    new SpellRestoration();
    new SpellSenseLife();
    new SpellUndeadProt();
    new SpellWaterwalk();
    new SpellIdentify();
    new SpellWallOfThorns();
    new SpellWallOfStone();
    new SpellWallOfIce();
    new SpellWallOfFire();
    new SpellWallOfIron();
    new SpellThornTrap();
    new SpellFierySheet();
    new SpellPower();
    new SpellIntellect();
    new SpellConfusion();
    new SpellFear();
    new SpellSatiation();
    new SpellQuench();
    new SpellConfidence();
    new SpellNopain();
    new SpellDermalHardening();
    new SpellWoundClosure();
    new SpellAntibody();
    new SpellRetina();
    new SpellAdrenaline();
    new SpellBreathingStasis();
    new SpellVertigo();
    new SpellMetabolism();
    new SpellEgoWhip();
    new SpellPsychicCrush();
    new SpellRelaxation();
    new SpellWeakness();
    new SpellFortressOfWill();
    new SpellCellRegen();
    new SpellPsishield();
    new SpellPsychicSurge();
    new SpellPsychicConduit();
    new SpellPsionicShatter();
    new SpellIdInsinuation();
    new SpellMelatonicFlood();
    new SpellMotorSpasm();
    new SpellPsychicResistance();
    new SpellMassHysteria();
    new SpellGroupConfidence();
    new SpellClumsiness();
    new SpellEndurance();
    new SpellNullpsi();
    new SpellTelepathy();
    new SpellDistraction();
    new SkillPsiblast();
    new SkillPsilocate();
    new SkillPsidrain();
    new SpellElectrostaticField();
    new SpellEntropyField();
    new SpellAcidity();
    new SpellAttractionField();
    new SpellNuclearWasteland();
    new SpellSpacetimeImprint();
    new SpellSpacetimeRecall();
    new SpellFluoresce();
    new SpellGammaRay();
    new SpellGravityWell();
    new SpellCapacitanceBoost();
    new SpellElectricArc();
    new SpellTemporalCompression();
    new SpellTemporalDilation();
    new SpellHalflife();
    new SpellMicrowave();
    new SpellOxidize();
    new SpellRandomCoordinates();
    new SpellRepulsionField();
    new SpellVacuumShroud();
    new SpellAlbedoShield();
    new SpellTransmittance();
    new SpellTimeWarp();
    new SpellRadioimmunity();
    new SpellDensify();
    new SpellLatticeHardening();
    new SpellChemicalStability();
    new SpellRefraction();
    new SpellNullify();
    new SpellAreaStasis();
    new SpellEmpPulse();
    new SpellFissionBlast();
    new SkillAmbush();
    new ZenHealing();
    new ZenAwareness();
    new ZenOblivity();
    new ZenMotion();
    new SpellTidalSpacewarp();
    new SkillCleave();
    new SkillStrike();
    new SkillHamstring();
    new SkillDrag();
    //new SkillSnatch();
    new SkillArchery();
    new SkillBowFletch();
    new SkillReadScrolls();
    new SkillUseWands();
    new SkillBackstab();
    new SkillCircle();
    new SkillHide();
    new SkillKick();
    new SkillBash();
    new SkillBreakDoor();
    new SkillHeadbutt();
    new SkillHotwire();
    new SkillGouge();
    new SkillStun();
    new SkillFeign();
    new SkillConceal();
    new SkillPlant();
    new SkillPickLock();
    new SkillRescue();
    new SkillSneak();
    new SkillSteal();
    new SkillTrack();
    new SkillPunch();
    new SkillPiledrive();
    new SkillShieldMastery();
    new SkillSleeper();
    new SkillElbow();
    new SkillKnee();
    new SkillAutopsy();
    new SkillBerserk();
    new SkillBattleCry();
    new SkillKia();
    new SkillCryFromBeyond();
    new SkillStomp();
    new SkillBodyslam();
    new SkillChoke();
    new SkillClothesline();
    new SkillTag();
    new SkillIntimidate();
    new SkillSpeedHealing();
    new SkillStalk();
    new SkillHearing();
    new SkillBearhug();
    new SkillBite();
    new SkillDblAttack();
    new SkillNightVision();
    new SkillTurn();
    new SkillAnalyze();
    new SkillEvaluate();
    new SkillCureDisease();
    new SkillHolyTouch();
    new SkillBandage();
    new SkillFirstaid();
    new SkillMedic();
    new SkillLeatherworking();
    new SkillMetalworking();
    new SkillConsider();
    new SkillGlance();
    new SkillShoot();
    new SkillBehead();
    new SkillEmpower();
    new SkillDisarm();
    new SkillSpinkick();
    new SkillRoundhouse();
    new SkillSidekick();
    new SkillSpinfist();
    new SkillJab();
    new SkillHook();
    new SkillSweepkick();
    new SkillTrip();
    new SkillUppercut();
    new SkillGroinkick();
    new SkillClaw();
    new SkillRabbitpunch();
    new SkillImpale();
    new SkillPeleKick();
    new SkillLungePunch();
    new SkillTornadoKick();
    new SkillTripleAttack();
    new SkillCounterAttack();
    new SkillSwimming();
    new SkillThrowing();
    new SkillRiding();
    new SkillAppraise();
    new SkillPipemaking();
    new SkillSecondWeapon();
    new SkillScanning();
    new SkillRetreat();
    new SkillGunsmithing();
    new SkillPistolwhip();
    new SkillCrossface();
    new SkillWrench();
    new SkillElusion();
    new SkillInfiltrate();
    new SkillShoulderThrow();
    new SkillProfPound();
    new SkillProfWhip();
    new SkillProfPierce();
    new SkillProfSlash();
    new SkillProfCrush();
    new SkillProfBlast();
    new SpellDecoy();
    new SkillGarotte();
    new SkillDemolitions();
    new SkillReconfigure();
    new SkillReboot();
    new SkillMotionSensor();
    new SkillStasis();
    new SkillEnergyField();
    new SkillReflexBoost();
    new SkillImplantW();
    new SkillOffensivePos();
    new SkillDefensivePos();
    new SkillMeleeCombatTac();
    new SkillNeuralBridging();
    new SkillNaniteReconstruction();
    //new SkillArterialFlow();
    new SkillOptimmunalResp();
    new SkillAdrenalMaximizer();
    new SkillPowerBoost();
    new SkillFastboot();
    new SkillSelfDestruct();
    new SkillBioscan();
    new SkillDischarge();
    new SkillSelfrepair();
    new SkillCyborepair();
    new SkillOverhaul();
    new SkillDamageControl();
    new SkillElectronics();
    new SkillHacking();
    new SkillCyberscan();
    new SkillCyboSurgery();
    new SkillHyperscan();
    new SkillOverdrain();
    new SkillEnergyWeapons();
    new SkillProjWeapons();
    new SkillSpeedLoading();
    new SkillPalmStrike();
    new SkillThroatStrike();
    new SkillWhirlwind();
    new SkillHipToss();
    new SkillCombo();
    new SkillDeathTouch();
    new SkillCraneKick();
    new SkillRidgehand();
    new SkillScissorKick();
    new SkillPinchAlpha();
    new SkillPinchBeta();
    new SkillPinchGamma();
    new SkillPinchDelta();
    new SkillPinchEpsilon();
    new SkillPinchOmega();
    new SkillPinchZeta();
    new SkillMeditate();
    new SkillKata();
    new SkillEvasion();
    new SkillFlying();
    new SkillSummon();
    new SkillFeed();
    new SkillBeguile();
    new SkillDrain();
    new SkillCreateVampire();
    new SkillControlUndead();
    new SkillTerrorize();
    new SkillLecture();
    new SkillEnergyConversion();
    new SpellHellFire();
    new SpellHellFrost();
    new SpellHellFireStorm();
    new SpellHellFrostStorm();
    new SpellTrogStench();
    new SpellFrostBreath();
    new SpellFireBreath();
    new SpellGasBreath();
    new SpellAcidBreath();
    new SpellLightningBreath();
    new SpellManaRestore();
    new SpellPetrify();
    new SpellSickness();
    new SpellEssenceOfEvil();
    new SpellEssenceOfGood();
    new SpellShadowBreath();
    new SpellSteamBreath();
    new SpellQuadDamage();
    new SpellZhengisFistOfAnnihilation();
    new SpellFrostBreathing();
    new SpellFireBreathing();
    new SpellVestigialRune();
    new SpellPrismaticSphere();
    new SpellQuantumRift();
    new SpellStoneskin();
    new SpellShieldOfRighteousness();
    new SpellSunRay();
    new SpellTaint();
    new SpellBlackmantle();
    new SpellSanctification();
    new SpellStigmata();
    new SpellDivinePower();
    new SpellDeathKnell();
    new SpellManaShield();
    new SpellEntangle();
    new SpellElementalBrand();
    new SpellSummonLegion();
    new SpellAntiMagicShell();
    new SpellWardingSigil();
    new SpellDivineIntervention();
    new SpellSphereOfDesecration();
    new SpellAmnesia();
    new ZenTranslocation();
    new ZenCelerity();
    new SkillDisguise();
    new SkillUncannyDodge();
    new SkillDeEnergize();
    new SkillAssimilate();
    new SkillRadionegation();
    new SpellMaleficViolation();
    new SpellRighteousPenetration();
    new SkillWormhole();
    new SpellGaussShield();
    new SpellUnholyStalker();
    new SpellAnimateDead();
    new SpellInferno();
    new SpellVampiricRegeneration();
    new SpellBanishment();
    new SkillDisciplineOfSteel();
    new SkillGreatCleave();
    new SpellLocustRegeneration();
    new SpellThornSkin();
    new SpellSpiritTrack();
    new SkillSnipe();
    new SkillAdvImplantW();
}
