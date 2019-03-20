// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.hosted.controller.athenz.impl;

import com.google.inject.Inject;
import com.yahoo.config.provision.ApplicationName;
import com.yahoo.log.LogLevel;
import com.yahoo.vespa.athenz.api.AthenzDomain;
import com.yahoo.vespa.athenz.api.AthenzIdentity;
import com.yahoo.vespa.athenz.api.AthenzPrincipal;
import com.yahoo.vespa.athenz.api.AthenzResourceName;
import com.yahoo.vespa.athenz.api.AthenzRole;
import com.yahoo.vespa.athenz.api.AthenzService;
import com.yahoo.vespa.athenz.api.OktaAccessToken;
import com.yahoo.vespa.athenz.client.zms.RoleAction;
import com.yahoo.vespa.athenz.client.zms.ZmsClient;
import com.yahoo.vespa.athenz.client.zts.ZtsClient;
import com.yahoo.vespa.hosted.controller.Application;
import com.yahoo.vespa.hosted.controller.api.integration.athenz.AthenzClientFactory;
import com.yahoo.vespa.hosted.controller.athenz.ApplicationAction;
import com.yahoo.vespa.hosted.controller.permits.ApplicationClaim;
import com.yahoo.vespa.hosted.controller.permits.AthenzApplicationClaim;
import com.yahoo.vespa.hosted.controller.permits.AthenzTenantClaim;
import com.yahoo.vespa.hosted.controller.permits.AccessControl;
import com.yahoo.vespa.hosted.controller.permits.TenantClaim;
import com.yahoo.vespa.hosted.controller.tenant.AthenzTenant;
import com.yahoo.vespa.hosted.controller.tenant.Tenant;
import com.yahoo.vespa.hosted.controller.tenant.UserTenant;

import javax.ws.rs.ForbiddenException;
import java.security.Principal;
import java.util.Arrays;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.logging.Logger;
import java.util.stream.Collectors;

/**
 * @author bjorncs
 * @author jonmv
 */
public class AthenzFacade implements AccessControl {

    private static final Logger log = Logger.getLogger(AthenzFacade.class.getName());
    private final ZmsClient zmsClient;
    private final ZtsClient ztsClient;
    private final AthenzService service;

    @Inject
    public AthenzFacade(AthenzClientFactory factory) {
        this(factory.createZmsClient(), factory.createZtsClient(), factory.getControllerIdentity());
    }

    public AthenzFacade(ZmsClient zmsClient, ZtsClient ztsClient, AthenzService identity) {
        this.zmsClient = zmsClient;
        this.ztsClient = ztsClient;
        this.service = identity;
    }

    @Override
    public Tenant createTenant(TenantClaim claim, List<Tenant> existing) {
        AthenzTenantClaim athenzClaim = (AthenzTenantClaim) claim;
        AthenzDomain domain = athenzClaim.domain()
                                         .orElseThrow(() -> new IllegalArgumentException("Must provide Athenz domain."));

        Tenant tenant = AthenzTenant.create(athenzClaim.tenant(),
                                            athenzClaim.domain()
                                                       .orElseThrow(() -> new IllegalArgumentException("Must provide Athenz domain.")),
                                            athenzClaim.property()
                                                       .orElseThrow(() -> new IllegalArgumentException("Must provide property.")),
                                            athenzClaim.propertyId());

        verifyIsDomainAdmin(((AthenzPrincipal) athenzClaim.user()).getIdentity(), domain);

        Optional<Tenant> existingWithSameDomain = existing.stream()
                                                          .filter(existingTenant ->    existingTenant.type() == Tenant.Type.athenz
                                                                                       && domain.equals(((AthenzTenant) existingTenant).domain()))
                                                          .findAny();

        if (existingWithSameDomain.isPresent()) { // Throw if domain is already taken.
            if ( ! existingWithSameDomain.get().name().equals(claim.tenant()))
                throw new IllegalArgumentException("Could not create tenant '" + athenzClaim.tenant().value() +
                                                   "': The Athens domain '" +
                                                   domain.getName() + "' is already connected to tenant '" +
                                                   existingWithSameDomain.get().name().value() + "'");
        }
        else { // Create tenant resources in Athenz if domain is not already taken.
            log("createTenancy(tenantDomain=%s, service=%s)", athenzClaim.domain(), service);
            zmsClient.createTenancy(domain, service, athenzClaim.token());
        }

        return tenant;
    }
    @Override
    public Tenant updateTenant(TenantClaim claim, List<Tenant> existing, List<Application> applications) {
        AthenzTenantClaim tenantClaim = (AthenzTenantClaim) claim;
        AthenzDomain domain = tenantClaim.domain()
                                          .orElseThrow(() -> new IllegalArgumentException("Must provide Athenz domain."));

        Tenant tenant = AthenzTenant.create(tenantClaim.tenant(),
                                            tenantClaim.domain()
                                                        .orElseThrow(() -> new IllegalArgumentException("Must provide Athenz domain.")),
                                            tenantClaim.property()
                                                        .orElseThrow(() -> new IllegalArgumentException("Must provide property.")),
                                            tenantClaim.propertyId());

        verifyIsDomainAdmin(((AthenzPrincipal) tenantClaim.user()).getIdentity(), domain);

        AthenzTenant oldTenant = existing.stream()
                                         .filter(existingTenant -> existingTenant.name().equals(claim.tenant()))
                                         .findAny()
                                         .map(AthenzTenant.class::cast)
                                         .orElseThrow(() -> new IllegalArgumentException("Cannot update a non-existent tenant."));

        Optional<Tenant> existingWithSameDomain = existing.stream()
                                                          .filter(existingTenant ->    existingTenant.type() == Tenant.Type.athenz
                                                                                    && domain.equals(((AthenzTenant) existingTenant).domain()))
                                                          .findAny();

        if (existingWithSameDomain.isPresent()) { // Throw if domain taken by someone else, or do nothing if taken by this tenant.
            if ( ! existingWithSameDomain.get().equals(oldTenant))
                throw new IllegalArgumentException("Could not create tenant '" + tenantClaim.tenant().value() +
                                                   "': The Athens domain '" +
                                                   domain.getName() + "' is already connected to tenant '" +
                                                   existingWithSameDomain.get().name().value() + "'");

            return tenant; // Short-circuit here if domain is still the same.
        }
        else { // Delete and recreate tenant, and optionally application, resources in Athenz otherwise.
            log("createTenancy(tenantDomain=%s, service=%s)", tenantClaim.domain(), service);
            zmsClient.createTenancy(domain, service, tenantClaim.token());
            for (Application application : applications)
                createApplication(domain, application.id().application(), tenantClaim.token());

            log("deleteTenancy(tenantDomain=%s, service=%s)", tenantClaim.domain(), service);
            for (Application application : applications)
                deleteApplication(oldTenant.domain(), application.id().application(), tenantClaim.token());
            zmsClient.deleteTenancy(oldTenant.domain(), service, tenantClaim.token());
        }

        return tenant;
    }

    @Override
    public void deleteTenant(TenantClaim claim, Tenant tenant) {
        AthenzTenantClaim athenzClaim = (AthenzTenantClaim) claim;
        AthenzDomain domain = ((AthenzTenant) tenant).domain();

        log("deleteTenancy(tenantDomain=%s, service=%s)", athenzClaim.domain(), service);
        zmsClient.deleteTenancy(domain, service, athenzClaim.token());
    }

    @Override
    public void createApplication(ApplicationClaim claim) {
        AthenzApplicationClaim athenzClaim = (AthenzApplicationClaim) claim;
        createApplication(athenzClaim.domain(), athenzClaim.application().application(), athenzClaim.token());
    }

    private void createApplication(AthenzDomain domain, ApplicationName application, OktaAccessToken token) {
        Set<RoleAction> tenantRoleActions = createTenantRoleActions();
        log("createProviderResourceGroup(" +
            "tenantDomain=%s, providerDomain=%s, service=%s, resourceGroup=%s, roleActions=%s)",
            domain, service.getDomain().getName(), service.getName(), application, tenantRoleActions);
        zmsClient.createProviderResourceGroup(domain, service, application.value(), tenantRoleActions, token);
    }

    @Override
    public void deleteApplication(ApplicationClaim claim) {
        AthenzApplicationClaim athenzClaim = (AthenzApplicationClaim) claim;
        log("deleteProviderResourceGroup(tenantDomain=%s, providerDomain=%s, service=%s, resourceGroup=%s)",
            athenzClaim.domain(), service.getDomain().getName(), service.getName(), athenzClaim.application());
        zmsClient.deleteProviderResourceGroup(athenzClaim.domain(), service, athenzClaim.application().application().value(), athenzClaim.token());
    }

    @Override
    public List<Tenant> accessibleTenants(List<Tenant> tenants, Principal principal) {
        AthenzIdentity identity = ((AthenzPrincipal) principal).getIdentity();
        List<AthenzDomain> userDomains = ztsClient.getTenantDomains(service, identity, "admin");
        return tenants.stream()
                      .filter(tenant ->    tenant instanceof UserTenant && ((UserTenant) tenant).is(identity.getName())
                                        || tenant instanceof AthenzTenant && userDomains.contains(((AthenzTenant) tenant).domain()))
                      .collect(Collectors.toUnmodifiableList());
    }

    private void deleteApplication(AthenzDomain domain, ApplicationName application, OktaAccessToken token) {
        log("deleteProviderResourceGroup(tenantDomain=%s, providerDomain=%s, service=%s, resourceGroup=%s)",
            domain, service.getDomain().getName(), service.getName(), application);
        zmsClient.deleteProviderResourceGroup(domain, service, application.value(), token);
    }

    public boolean hasApplicationAccess(
            AthenzIdentity identity, ApplicationAction action, AthenzDomain tenantDomain, ApplicationName applicationName) {
        return hasAccess(
                action.name(), applicationResourceString(tenantDomain, applicationName), identity);
    }

    public boolean hasTenantAdminAccess(AthenzIdentity identity, AthenzDomain tenantDomain) {
        return hasAccess(TenantAction._modify_.name(), tenantResourceString(tenantDomain), identity);
    }

    public boolean hasHostedOperatorAccess(AthenzIdentity identity) {
        return hasAccess("modify", service.getDomain().getName() + ":hosted-vespa", identity);
    }

    /**
     * Used when creating tenancies. As there are no tenancy policies at this point,
     * we cannot use {@link #hasTenantAdminAccess(AthenzIdentity, AthenzDomain)}
     */
    private void verifyIsDomainAdmin(AthenzIdentity identity, AthenzDomain domain) {
        log("getMembership(domain=%s, role=%s, principal=%s)", domain, "admin", identity);
        if ( ! zmsClient.getMembership(new AthenzRole(domain, "admin"), identity))
            throw new ForbiddenException(
                    String.format("The user '%s' is not admin in Athenz domain '%s'", identity.getFullName(), domain.getName()));
    }

    public List<AthenzDomain> getDomainList(String prefix) {
        log.log(LogLevel.DEBUG, String.format("getDomainList(prefix=%s)", prefix));
        return zmsClient.getDomainList(prefix);
    }

    private static Set<RoleAction> createTenantRoleActions() {
        return Arrays.stream(ApplicationAction.values())
                .map(action -> new RoleAction(action.roleName, action.name()))
                .collect(Collectors.toSet());
    }

    private boolean hasAccess(String action, String resource, AthenzIdentity identity) {
        log("getAccess(action=%s, resource=%s, principal=%s)", action, resource, identity);
        return zmsClient.hasAccess(AthenzResourceName.fromString(resource), action, identity);
    }

    private static void log(String format, Object... args) {
        log.log(LogLevel.DEBUG, String.format(format, args));
    }

    private String resourceStringPrefix(AthenzDomain tenantDomain) {
        return String.format("%s:service.%s.tenant.%s",
                             service.getDomain().getName(), service.getName(), tenantDomain.getName());
    }

    private String tenantResourceString(AthenzDomain tenantDomain) {
        return resourceStringPrefix(tenantDomain) + ".wildcard";
    }

    private String applicationResourceString(AthenzDomain tenantDomain, ApplicationName applicationName) {
        return resourceStringPrefix(tenantDomain) + "." + "res_group" + "." + applicationName.value() + ".wildcard";
    }

    private enum TenantAction {
        // This is meant to match only the '*' action of the 'admin' role.
        // If needed, we can replace it with 'create', 'delete' etc. later.
        _modify_
    }

}
